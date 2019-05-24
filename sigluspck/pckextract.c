#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "compression.c"
#include <windows.h>

static unsigned char key2[256] =
{
    0x70, 0xF8, 0xA6, 0xB0, 0xA1, 0xA5, 0x28, 0x4F, 0xB5, 0x2F, 0x48, 0xFA, 0xE1, 0xE9, 0x4B, 0xDE,
    0xB7, 0x4F, 0x62, 0x95, 0x8B, 0xE0, 0x03, 0x80, 0xE7, 0xCF, 0x0F, 0x6B, 0x92, 0x01, 0xEB, 0xF8,
    0xA2, 0x88, 0xCE, 0x63, 0x04, 0x38, 0xD2, 0x6D, 0x8C, 0xD2, 0x88, 0x76, 0xA7, 0x92, 0x71, 0x8F,
    0x4E, 0xB6, 0x8D, 0x01, 0x79, 0x88, 0x83, 0x0A, 0xF9, 0xE9, 0x2C, 0xDB, 0x67, 0xDB, 0x91, 0x14,
    0xD5, 0x9A, 0x4E, 0x79, 0x17, 0x23, 0x08, 0x96, 0x0E, 0x1D, 0x15, 0xF9, 0xA5, 0xA0, 0x6F, 0x58,
    0x17, 0xC8, 0xA9, 0x46, 0xDA, 0x22, 0xFF, 0xFD, 0x87, 0x12, 0x42, 0xFB, 0xA9, 0xB8, 0x67, 0x6C,
    0x91, 0x67, 0x64, 0xF9, 0xD1, 0x1E, 0xE4, 0x50, 0x64, 0x6F, 0xF2, 0x0B, 0xDE, 0x40, 0xE7, 0x47,
    0xF1, 0x03, 0xCC, 0x2A, 0xAD, 0x7F, 0x34, 0x21, 0xA0, 0x64, 0x26, 0x98, 0x6C, 0xED, 0x69, 0xF4,
    0xB5, 0x23, 0x08, 0x6E, 0x7D, 0x92, 0xF6, 0xEB, 0x93, 0xF0, 0x7A, 0x89, 0x5E, 0xF9, 0xF8, 0x7A,
    0xAF, 0xE8, 0xA9, 0x48, 0xC2, 0xAC, 0x11, 0x6B, 0x2B, 0x33, 0xA7, 0x40, 0x0D, 0xDC, 0x7D, 0xA7,
    0x5B, 0xCF, 0xC8, 0x31, 0xD1, 0x77, 0x52, 0x8D, 0x82, 0xAC, 0x41, 0xB8, 0x73, 0xA5, 0x4F, 0x26,
    0x7C, 0x0F, 0x39, 0xDA, 0x5B, 0x37, 0x4A, 0xDE, 0xA4, 0x49, 0x0B, 0x7C, 0x17, 0xA3, 0x43, 0xAE,
    0x77, 0x06, 0x64, 0x73, 0xC0, 0x43, 0xA3, 0x18, 0x5A, 0x0F, 0x9F, 0x02, 0x4C, 0x7E, 0x8B, 0x01,
    0x9F, 0x2D, 0xAE, 0x72, 0x54, 0x13, 0xFF, 0x96, 0xAE, 0x0B, 0x34, 0x58, 0xCF, 0xE3, 0x00, 0x78,
    0xBE, 0xE3, 0xF5, 0x61, 0xE4, 0x87, 0x7C, 0xFC, 0x80, 0xAF, 0xC4, 0x8D, 0x46, 0x3A, 0x5D, 0xD0,
    0x36, 0xBC, 0xE5, 0x60, 0x77, 0x68, 0x08, 0x4F, 0xBB, 0xAB, 0xE2, 0x78, 0x07, 0xE8, 0x73, 0xBF
};

//static unsigned char key1[16] = { 0x5A, 0x62, 0x84, 0x05, 0x5F, 0xE8, 0x46, 0x97, 0xC5, 0x6C, 0xF8, 0x0C, 0x85, 0x53, 0xCF, 0x50 }; //SummerPockets TRIAL
//#define WTFVAL 0x266 //summerpockets TRIAL

static unsigned char key1[16] = { 0x7E, 0x23, 0xBA, 0x0A, 0x4B, 0xA8, 0x29, 0x9A, 0xF6, 0x7C, 0xFD, 0x19, 0x88, 0x34, 0xD1, 0x77  }; //SummerPockets FULL
#define WTFVAL 0x566 //summerpockets FULL

typedef struct { int offset, size; } PAIRVAL;

typedef struct
{
    int hdrlen;
    PAIRVAL table1;
    PAIRVAL globalvar;
    PAIRVAL globalvarstr;
    PAIRVAL name1;
    PAIRVAL name2;
    PAIRVAL name3;
    PAIRVAL name4;
    PAIRVAL fnamestr;
    PAIRVAL filetoc;
    PAIRVAL data;
    int encrypt2;
    int wtf;
} PCKHDR;

static const char *sections[] = { "table1",  "gvar", "gvarstr", "name1", "name2", "name3", "name4", "fname" };

void DecompressData(unsigned char *inbuffer, unsigned char *outbuffer, int decomplen)
{
    //unsigned char *origout = outbuffer;
    unsigned char *endbuf = outbuffer + decomplen;

    while (outbuffer < endbuf)
    {
        short i = 8, flag = *inbuffer++;

        for (; i > 0 && outbuffer != endbuf; i--, inbuffer++)
        {
            if(flag & 1) { *outbuffer++ = *inbuffer; }
            else
            {
                unsigned short ref = *(unsigned short*)inbuffer++;
                int len = (ref & 0x0F) + 2;
                ref >>= 4;

                while(len-- > 0)
                {
                    *outbuffer = *(outbuffer - ref);
                    outbuffer++;
                }
            }

            flag >>= 1;
        }
    }

    //printf(">>>> Decompressed: %X of %X bytes\n",outbuffer - origout, decomplen);
}


#if 1
unsigned char *CompressData(unsigned char *inbuffer, int len, int *complen, int compression);

void *compress(unsigned char *src, int srclen)
{
    //use compression code
    int complen;

    return CompressData((unsigned char*)src, srclen, &complen, 1);
}
#else
void *compress(unsigned char *src, int srclen)
{
    //dummy packer
    unsigned char *origcbuf, *compbuf;
    int i;

    compbuf = (unsigned char*)malloc(srclen * 2);
    origcbuf = compbuf;

    for (i = 0, compbuf += 8; i < srclen; i++)
    {
        if ((i % 8) == 0) { *compbuf++ = 0xFF; }
        *compbuf++ = *src++;
    }

    ((int*)origcbuf)[0] = compbuf - origcbuf;
    ((int*)origcbuf)[1] = srclen;
    return origcbuf;
}
#endif

void decrypt(unsigned char *data, int len, int encrypt2)
{
    int j;

    for (j = 0; j < len; j++) { data[j] ^= key2[j % 256]; }

    if (encrypt2 == 1)
    {
        for (j = 0; j < len; j++) { data[j] ^= key1[j % 16]; }
    }
}

int readfile(const char *fname, unsigned char **buf, int *fsize, int wide)
{
    FILE *infile;
    unsigned char *_buf;
    int readlen;

    if (wide == 1) { infile = _wfopen((wchar_t*)fname, L"rb"); }
    else { infile = fopen(fname, "rb"); }

    if (infile == NULL)
    {
        printf("error opening %s\n", fname);
        *fsize = 0;
        *buf = NULL;
        return 1;
    }

    fseek(infile, 0, SEEK_END);
    readlen = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    _buf = (unsigned char*)malloc(readlen);
    fread(_buf, 1, readlen, infile);
    fclose(infile);

    *fsize = readlen;
    *buf = _buf;
    return 0;
}

void dumptable(const char *outname, unsigned char *buf, PAIRVAL *v)
{
    FILE *outfile;
    char namepath[32];

    sprintf(namepath, "%s.bin", outname);
    outfile = fopen(namepath, "wb");
    if (outfile != NULL)
    {
        fwrite(&v[0].size, 1, 4, outfile);
        fwrite(buf + v[0].offset, 1, v[1].offset - v[0].offset, outfile);
        fclose(outfile);
    }
    else
    {
        printf("error creating %s\n", outname);
    }
}

void writetable(const char *fname, FILE *outfile, PAIRVAL *v)
{
    char namepath[32];
    unsigned char *buf;
    int fsize;

    sprintf(namepath, "%s.bin", fname);
    readfile(namepath, &buf, &fsize, 0);

    v[0].offset = ftell(outfile);
    v[0].size = *(int*)buf;

    fwrite(buf + 4, 1, fsize - 4, outfile);
    free(buf);
}

void extract(const char *pckname)
{
    unsigned char *buf;
    PCKHDR *hdr;
    int fsize, i;

    PAIRVAL *toc, *fntable;

    i = readfile(pckname, &buf, &fsize, 0);
    if (i != 0) { return; }

    hdr = (PCKHDR*)buf;
    toc = (PAIRVAL*)(buf + hdr->filetoc.offset);
    fntable = (PAIRVAL*)(buf + hdr->name4.offset);

    if (hdr->hdrlen != sizeof(PCKHDR))
    {
        printf("wrong header size (%d / expected %d\n", hdr->hdrlen, sizeof(PCKHDR));
        goto leave;
    }

    //begin extracting
    for (i = 0; i < hdr->filetoc.size; i++)
    {
        FILE *outfile;
        wchar_t fname[128];
        char sjis[256];
        unsigned char *data, *outdata;

        memcpy(fname, buf + hdr->fnamestr.offset + fntable[i].offset * 2, fntable[i].size * 2);
        memcpy(fname + fntable[i].size, L".ss", 8);

        WideCharToMultiByte(932, 0, fname, -1, sjis, sizeof(sjis), "", NULL);
        printf("%s | offset=0x%X | len = %d\n", sjis, toc[i].offset, toc[i].size);

        data = (unsigned char*)buf + hdr->data.offset + toc[i].offset;

        decrypt(data, toc[i].size, hdr->encrypt2);
        //decrypt(data, toc[i].size, 0);

        printf("  complen = %d / declen = %d\n", *(int*)data, *(int*)(data + 4));
        if (*(int*)data != toc[i].size) { printf(" ||| SIZE MISMATCH!!\n"); }

        outdata = (unsigned char*)malloc(*(int*)(data + 4));

        DecompressData(data + 8, outdata, *(int*)(data + 4));

        outfile = _wfopen(fname, L"wb");
        if (outfile != NULL)
        {
            fwrite(outdata, 1, *(int*)(data + 4), outfile);
            //fwrite(data, 1, toc[i].size, outfile);
            fclose(outfile);
        }
        else
        {
            printf("error creating %s\n", sjis);
        }

        free(outdata);
    }

    //other stuff
    for (i = 0; i < (int)(sizeof(sections) / sizeof(*sections)); i++)
    {
        PAIRVAL *v = &hdr->table1;
        dumptable(sections[i], buf, v + i);
    }

leave:
    free(buf);
}


void rebuild(const char *indir, const char *outname)
{
    FILE *outfile;
    int ret, numfiles, i;

    PCKHDR hdr;
    wchar_t *names;
    PAIRVAL *filetoc, *nameidx;
    unsigned char *tb1, *tb2;

    outfile = fopen(outname, "wb");
    if (outfile == NULL)
    {
        printf("error creating %s\n", outname);
        return;
    }

    ret = _chdir(indir);
    if (ret != 0) { printf("error opening directory %s\n", indir); return; }

    //init header
    hdr.hdrlen = sizeof(PCKHDR);
    hdr.encrypt2 = 1;
    hdr.wtf = WTFVAL;

    fwrite(&hdr, 1, sizeof(hdr), outfile); //initial

    //load misc tables
    for (i = 0; i < (int)(sizeof(sections) / sizeof(*sections)); i++)
    {
        PAIRVAL *v = &hdr.table1;
        writetable(sections[i], outfile, v + i);
    }

    numfiles = hdr.fnamestr.size;

    hdr.filetoc.offset = ftell(outfile);
    hdr.filetoc.size = numfiles;
    filetoc = (PAIRVAL*)malloc(numfiles * sizeof(PAIRVAL));

    fwrite(filetoc, 1, numfiles * sizeof(PAIRVAL), outfile); //initial

    hdr.data.offset = ftell(outfile);
    hdr.data.size = numfiles;

    //prep
    readfile("fname.bin", &tb1, &i, 0);
    readfile("name4.bin", &tb2, &i, 0);
    names = (wchar_t*)(tb1 + 4);
    nameidx = (PAIRVAL*)(tb2 + 4);

    //write data
    for (i = 0; i < numfiles; i++)
    {
        wchar_t fname[40];
        char sjis[80];
        unsigned char *readbuf, *compbuf;
        int readlen;

        memcpy(fname, names + nameidx[i].offset, nameidx[i].size * 2);
        fname[nameidx[i].size] = 0;

        wcscat(fname, L".ss");

        WideCharToMultiByte(932, 0, fname, -1, sjis, sizeof(sjis), "", NULL);
        printf("%s\n", sjis);

        readfile((char*)fname, &readbuf, &readlen, 1);
        compbuf = (unsigned char*) compress(readbuf, readlen);

        filetoc[i].offset = ftell(outfile) - hdr.data.offset;
        filetoc[i].size = *(int*)compbuf;

        decrypt(compbuf, filetoc[i].size, hdr.encrypt2);
        fwrite(compbuf, 1, filetoc[i].size, outfile);

        free(readbuf);
        free(compbuf);
    }

    //finalize
    fflush(outfile);

    fseek(outfile, 0, SEEK_SET);
    fwrite(&hdr, 1, sizeof(PCKHDR), outfile);
    fflush(outfile);

    fseek(outfile, hdr.filetoc.offset, SEEK_SET);
    fwrite(filetoc, 1, numfiles * sizeof(PAIRVAL), outfile);

    fclose(outfile);

    free(filetoc);
    free(tb1);
    free(tb2);
}

int main(int argc, char **argv)
{
    if (argc == 3 && argv[1][0]=='x') { extract(argv[2]); }
    else if (argc == 4 && argv[1][0]=='r') { rebuild(argv[2], argv[3]); }
    else { printf("\nusage:\n   siglusPCK x <Scene.pck>\n   siglusPCK r <input folder> <output.pck>\n"); }

    return 0;
}
