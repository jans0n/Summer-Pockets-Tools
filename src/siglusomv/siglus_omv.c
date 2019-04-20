#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "theora/theoradec.h"


static th_info           ti;
static th_comment        tc;
static th_setup_info    *ts;
static th_dec_ctx       *td;

static int theora_p=0;
static int theora_processing_headers;
static int stateflag=0;

static th_ycbcr_buffer ycbcr;




typedef struct
{
    int hdrlen;
    int ver;

    int zero[8];
    int wtf1; //colordepth?
    int width;
    int height;
    int zero1;

    int zero2;
    int frametime; //microseconds, inverse of framerate
    int serial; //ogg stream serialno
    int zero3;
    int wtf2;
    int pages_count;
    int frames_count;

    int zero4[7];
    int unkkk[7];
    int zero5[7];
} OMV_HDR;

//index for Ogg pages
typedef struct
{
    unsigned int idx;
    unsigned int wtf;
    unsigned int len;
    unsigned int offset;
    int idx_start;
    int numframes; //frames in packet
    int framestart;
} OMV_PAGEIDX;

//index for frame timings and corresponding ogg pages
typedef struct
{
    int frame;
    int pageidx;
    int seqcount; //frame idx within page
    int wtf;
    int keyframe; //typically multiple of 16
    int keyframe_pageidx;
    int start; //time in millisec
    int end; //time in millisec
} OMV_FRAMEIDX;


typedef struct
{
    ogg_sync_state *oy;
    ogg_stream_state *to;

    unsigned int offset;
    long page_check;
    unsigned int frame;
    unsigned int subframe;

    unsigned int keyframe, keyframepage;

    float frametime;

    OMV_HDR omvhdr;
    OMV_PAGEIDX *idxpage;
    OMV_FRAMEIDX *idxframe;
} STREAMSTAT;

static void init_streamstat(STREAMSTAT *s, ogg_sync_state *oy, ogg_stream_state *to)
{
    //indexing
    s->oy = oy;
    s->to = to;
    s->offset = 0;
    s->page_check = 0;
    s->keyframe = 0;
    s->frame = 0;
    //s->frametime = (float)ti.fps_denominator / (float)ti.fps_numerator;

    s->to->pageno = 0;

    s->idxpage = (OMV_PAGEIDX*)malloc(sizeof(OMV_PAGEIDX) * 32*1024);
    s->idxframe = (OMV_FRAMEIDX*)malloc(sizeof(OMV_FRAMEIDX) * 32*1024);
}
static void uninit_streamstat(STREAMSTAT *s)
{
    free(s->idxpage);
    free(s->idxframe);
}
static void setpage(STREAMSTAT *s)
{
    OMV_PAGEIDX *p;
    printf("\npage %lu, pos %u, len %u\n", s->to->pageno, s->offset, s->oy->returned);

    if (s->page_check != s->to->pageno)
    {
        printf("wtf, mispatch page number | expected %ld\n", s->page_check);
        exit(1);
    }

    //set up page entry
    p = s->idxpage + s->page_check;
    p->idx = s->page_check;
    p->wtf = 256;
    p->len = s->oy->returned;
    p->offset = s->offset;
    p->idx_start = 0; //p->idx; ///wip
    p->numframes = 0;
    p->framestart = -1;

    /*if (p->idx > 0)
    {
        if (p[-1].framestart < 0)
        {
            p->wtf = 0;
            p->idx_start--;
        }
    }*/

    //stat
    s->offset += s->oy->returned;
    s->page_check++;
    s->subframe = 0;
}
static void setframe(STREAMSTAT *s)
{
    OMV_PAGEIDX *p;
    OMV_FRAMEIDX *f;

    //printf("%u ", s->frame);

    //page entry
    p = s->idxpage + s->page_check - 1;
    if (p->framestart < 0)
        p->framestart = s->frame;

    p->numframes++;

    //set up frame entry
    f = s->idxframe + s->frame;

    f->frame = s->frame;
    f->pageidx = s->page_check - 1;
    f->seqcount = s->subframe;
    f->wtf = 0; //1629440;
    f->keyframe = s->keyframe;
    f->keyframe_pageidx = s->keyframepage;

    f->start = round((float)s->frame * s->frametime);
    f->end = round(f->start + s->frametime);

    if (s->frame > 0)
    {
        OMV_FRAMEIDX *fprev = f - 1;
        if (f->start == fprev->end) f->start++;
    }

    if (f->frame == f->keyframe) { f->wtf |= 1; }

    //stat
    s->frame++;
    s->subframe++;
}

static void omv_setup(STREAMSTAT *s)
{
    OMV_HDR *h = &s->omvhdr;
    memset(h, 0, sizeof(OMV_HDR));

    h->hdrlen = sizeof(OMV_HDR);
    h->ver = 0x101;
    h->wtf1 = 2; ///?????
    h->width = ti.frame_width;
    h->height = ti.frame_height;

    h->frametime = ((float)ti.fps_denominator / (float)ti.fps_numerator * 1000000.0f);
    h->serial = s->to->serialno;
    h->wtf2 = 1;
    h->pages_count = s->page_check;
    h->frames_count = s->frame;

    h->unkkk[2] = -1;
    h->unkkk[3] = -1;
}







static void stripe_decoded(th_ycbcr_buffer _dst,th_ycbcr_buffer _src, int _fragy0,int _fragy_end)
{
}

static void open_video(void)
{
    th_stripe_callback cb;
    int pli;

    for(pli=0; pli<3; pli++)
    {
        int xshift;
        int yshift;
        xshift=pli!=0&&!(ti.pixel_fmt&1);
        yshift=pli!=0&&!(ti.pixel_fmt&2);
        ycbcr[pli].data=(unsigned char *)malloc((ti.frame_width>>xshift)*(ti.frame_height>>yshift)*sizeof(char));
        ycbcr[pli].stride=ti.frame_width>>xshift;
        ycbcr[pli].width=ti.frame_width>>xshift;
        ycbcr[pli].height=ti.frame_height>>yshift;
    }

    cb.ctx=ycbcr;
    cb.stripe_decoded=(th_stripe_decoded_func)stripe_decoded;
    th_decode_ctl(td,TH_DECCTL_SET_STRIPE_CB,&cb,sizeof(cb));
}


static int buffer_data(FILE *in,ogg_sync_state *oy)
{
    const int readlen=16;
    char *buffer=ogg_sync_buffer(oy,readlen);
    int bytes=fread(buffer,1,readlen,in);
    ogg_sync_wrote(oy,bytes);
    return bytes;
}
static int queue_page(ogg_stream_state *to, ogg_page *page)
{
    if(theora_p)
        ogg_stream_pagein(to,page);
    return 0;
}

static void omv_filegen(const char *fname_out, FILE *fpin, STREAMSTAT *s)
{
    FILE *outfile;
    unsigned char readbuf[1024];

    outfile = fopen(fname_out, "wb");
    if (outfile == NULL)
    {
        printf("error creating %s\n", fname_out);
        return;
    }

    s->idxpage[0].numframes = -1;
    s->idxpage[1].numframes = -1;

    omv_setup(s);

    printf("\n\n -- writing final output to %s\n", fname_out);
    fwrite(&s->omvhdr, 1, sizeof(OMV_HDR), outfile);
    fwrite(s->idxpage, 1, sizeof(OMV_PAGEIDX) * s->page_check, outfile);
    fwrite(s->idxframe, 1, sizeof(OMV_FRAMEIDX) * s->frame, outfile);

#if 1
    fseek(fpin, 0, SEEK_SET);
    while (1)
    {
        int rlen = fread(readbuf, 1, sizeof(readbuf), fpin);
        if (rlen <= 0) { break; }
        fwrite(readbuf, 1, rlen, outfile);
    }
#endif
    fclose(outfile);
}



void pack_omv(const char *fname_in, const char *fname_out)
{
    ogg_sync_state oy;
    ogg_page og;
    ogg_stream_state to;
    ogg_packet op;

    int videobuf_ready=0;
    ogg_int64_t videobuf_granulepos=-1;
    double videobuf_time=0;

    STREAMSTAT s;
    FILE *infile = NULL;

    infile=fopen(fname_in,"rb");
    if(infile==NULL)
    {
        printf("error opening %s\n", fname_in);
        exit(1);
    }

    ogg_sync_init(&oy);

    th_comment_init(&tc);
    th_info_init(&ti);

    init_streamstat(&s, &oy, &to);

    while(!stateflag)
    {
        int ret=buffer_data(infile,&oy);
        if(ret==0)
            break;
        while(ogg_sync_pageout(&oy,&og)>0)
        {
            int got_packet;
            ogg_stream_state test;

            setpage(&s);

            if(!ogg_page_bos(&og))
            {
                //don't leak the page; get it into the appropriate stream
                queue_page(&to, &og);
                stateflag=1;
                break;
            }

            ogg_stream_init(&test,ogg_page_serialno(&og));
            ogg_stream_pagein(&test,&og);
            got_packet = ogg_stream_packetpeek(&test,&op);

            if((got_packet==1) && !theora_p && (theora_processing_headers=th_decode_headerin(&ti,&tc,&ts,&op))>=0)
            {
                memcpy(&to,&test,sizeof(test));
                theora_p=1;

                //header ok
                if(theora_processing_headers)
                    ogg_stream_packetout(&to,NULL);
            }
            else { ogg_stream_clear(&test); }
        }
        // fall through to non-bos page parsing
    }

    while(theora_p && theora_processing_headers)
    {
        int ret;

        while(theora_processing_headers)
        {
            ret = ogg_stream_packetpeek(&to,&op);
            if (ret==0) { break; }
            else if(ret<0) { continue; }

            theora_processing_headers=th_decode_headerin(&ti,&tc,&ts,&op);
            if(theora_processing_headers<0)
            {
                printf("Error parsing Theora stream headers; corrupt stream?\n");
                exit(1);
            }
            else if(theora_processing_headers>0)
            {
                //Advance past the successfully processed header.
                ogg_stream_packetout(&to,NULL);
            }
            theora_p++;
        }

        if(!(theora_p && theora_processing_headers)) break;

        if(ogg_sync_pageout(&oy,&og)>0)
        {
            setpage(&s);
            queue_page(&to, &og);
        }
        else
        {
            int ret=buffer_data(infile,&oy);
            if(ret==0)
            {
                printf("End of file while searching for codec headers.\n");
                exit(1);
            }
        }
    }

    //codecs ready
    if(theora_p)
    {
        td=th_decode_alloc(&ti,ts);
        //printf("Ogg logical stream %lx is Theora %dx%d %.02f fps video\n" "Encoded frame content is %dx%d with %dx%d offset\n", to.serialno,ti.frame_width,ti.frame_height, (double)ti.fps_numerator/ti.fps_denominator, ti.pic_width,ti.pic_height,ti.pic_x,ti.pic_y);

        s.frametime = (float)ti.fps_denominator / (float)ti.fps_numerator * 1000.0f;
        //omv_setup(&s); //DEBUG ONLY
    }
    else
    {
        th_info_clear(&ti);
        th_comment_clear(&tc);
    }
    th_setup_free(ts);

    if(theora_p) open_video();

    if(1)
    {
        static const char *CHROMA_TYPES[4]= {"420jpeg",NULL,"422","444"};
        if(ti.pixel_fmt>=4||ti.pixel_fmt==TH_PF_RSVD)
        {
            printf("Unknown pixel format: %i\n",ti.pixel_fmt);
            exit(1);
        }
        //printf("YUV4MPEG2 C%s %dx%d F%d/%d I%c A%d:%d\n", CHROMA_TYPES[ti.pixel_fmt],ti.frame_width,ti.frame_height, ti.fps_numerator,ti.fps_denominator,'p', ti.aspect_numerator,ti.aspect_denominator);
    }

    ///main loop
    stateflag=0; //playback has not begun
    //queue any remaining pages from data we buffered but that did not contain headers
    //while(ogg_sync_pageout(&oy,&og)>0) { queue_page(&to, &og); }

    while(1)
    {
        while(theora_p && !videobuf_ready)
        {
            if(ogg_stream_packetout(&to,&op)>0)
            {
                if(th_decode_packetin(td,&op,&videobuf_granulepos)>=0)
                {
                    if (th_packet_iskeyframe(&op))
                    {
                        s.keyframe = s.frame;
                        s.keyframepage = s.page_check - 1;
                        //printf("k");
                    }

                    setframe(&s);

                    videobuf_time=th_granule_time(td,videobuf_granulepos);
                    videobuf_ready=1;
                }
            }
            else
                break;
        }

        if(!videobuf_ready && feof(infile))
            break;

        if(!videobuf_ready)
        {
            //no data yet for somebody.  Grab another page
            buffer_data(infile,&oy);
            while(ogg_sync_pageout(&oy,&og)>0)
            {
                //printf("\nogg page %ld | %d byte | FRAME %d\n", to.pageno, oy.returned, frames);
                setpage(&s);
                queue_page(&to, &og);
            }
        }

        videobuf_ready=0;
    }

    //output omv file
    omv_filegen(fname_out, infile, &s);

    //cleanup
    if(theora_p)
    {
        ogg_stream_clear(&to);
        th_decode_free(td);
        th_comment_clear(&tc);
        th_info_clear(&ti);
    }
    ogg_sync_clear(&oy);

    fclose(infile);

    uninit_streamstat(&s);

    //printf("\n\n%d frames\n", s.frame);
    printf("\nDone.\n");
}


int main(int argc,char **argv)
{
    if (argc < 3)
    {
        printf("\nusage:\n"
               "   siglus_omv <input.ogv> <output.omv>\n"
               );
        return 1;
    }

    pack_omv(argv[1], argv[2]);

    return 0;
}
