#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "compression.h"

#define LOOKBACK 4096 // size of the sliding window buffer
//#define LEVEL 17 // maximum amount of bytes to accept for compression
#define LEVEL 17 // maximum amount of bytes to accept for compression
#define ACCEPTLEVEL 3 // lowest amount of bytes to accept for compression

#define DECOMPDEBUG 0
#define COMPDEBUG 0

int OffTable[LEVEL+1];

static void GenerateTable(unsigned char *buffer, int len);
static int SearchData(unsigned char *buffer, int datalen, unsigned char *comp, int complen, int *outlen);

unsigned char *CompressData(unsigned char *inbuffer, int len, int *complen, int compression)
{
	unsigned char *output = (unsigned char*)calloc(len*4,sizeof(unsigned char)), *outptr = output;
	int outlen = 0, i = 0, curlen = 0;

	output += 8;

	while(curlen<=len)
	{
		unsigned char *ctrlbyte = output;

		outlen++, output++;

		for(i=0; i<8; i++)
		{
			int offset = 0, mlen = 0, tcurlen = curlen;

			if(tcurlen>0xfff)
				tcurlen = 0xfff;

			if(compression)
			{
				if(curlen+LEVEL>len) // LEVEL can be 2 minimum or 17 maximum
				{
					GenerateTable(inbuffer, LEVEL-((curlen+LEVEL)-len));
					offset = SearchData(inbuffer, tcurlen, inbuffer, LEVEL-((curlen+LEVEL)-len), &mlen);
				}
				else
				{
					GenerateTable(inbuffer, LEVEL);
					offset = SearchData(inbuffer, tcurlen, inbuffer, LEVEL, &mlen);
				}
			}

			if(offset==0)
			{
				unsigned char bitmask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
				//int x=0;

				*output++ = *inbuffer++, curlen++;
				*ctrlbyte |= bitmask[i];
			}
			else
			{
				unsigned short out = (offset<<4) | (mlen&0x0f);

				*(unsigned short*)output++ = out;

				curlen += mlen+2, inbuffer += mlen+2;
				output++, outlen++;
			}

			if(curlen>=len)
				break;

			outlen++;
		}

	}

	outlen += 8;

	*(int*)outptr = outlen;
	*(int*)(outptr+4) = len;

	*complen = outlen;
	return outptr;
}

/*
	My implementation of Knuth-Morris-Pratt's searching algorithm, based on what I read from Wikipedia.

	The first step is to generate the table, which creates a table that lists substrings in the input string.

	The second step is the search. It compares the current buffers like normal but then it jumps if it
	finds a mismatch.
*/

static void GenerateTable(unsigned char *buffer, int len)
{
	int i=0,x=0;

	memset(OffTable,'\0',LEVEL+1);

	OffTable[0] = -1;
	OffTable[1] = 0;

	for(i=2; i<len; )
	{
		if(buffer[i-1]==buffer[x])
		{
			OffTable[i] = x + 1;
			i++,x++;
		}
		else if(x>0)
		{
			x = OffTable[x];
		}
		else
		{
			OffTable[i] = 0;
			i++;
		}
	}
}

static int SearchData(unsigned char *buffer, int datalen, unsigned char *comp, int complen, int *outlen)
{
	unsigned char *ptr = buffer-datalen;
	int i=0, x=0;
	int matchlen=0,offset=0;

	if(complen-2<=0 || datalen<=complen)
		return 0;

	while(x < datalen)
	{
		if(comp[i]==ptr[i+x])
		{
			i++;

			if(i==complen)
			{
				matchlen = i;
				offset = x;
				break;
			}
		}
		else
		{
			if(i>matchlen)
			{
				matchlen = i;
				offset = x;
			}

			x += i-OffTable[i];

			if(OffTable[i] > 0)
				i = OffTable[i];
			else
				i = 0;
		}
	}

	if(matchlen<ACCEPTLEVEL)
		matchlen = 0, offset = 0;
	else
		offset = (int)buffer-((int)ptr+offset);

	if(matchlen>2)
		matchlen -= 2;

	#if COMPDEBUG == 1
	if(offset!=0 && matchlen!=0)
	{
		//printf("%02x   %08x [%08x] %08x [%08x]\n",(matchlen+2),(int)buffer,*(unsigned int*)buffer,(int)buffer-offset,*(unsigned int*)(buffer-offset));
		printf("%d %04x\n%08x ",(matchlen+2),offset,(int)buffer);
		for(i=0; i<complen; i++)
			printf("%02x ",comp[i]);
		printf("\n%08x ",(int)buffer-offset);
		for(i=0; i<complen; i++)
			printf("%02x ",(unsigned char*)(buffer-offset)[i]);
		printf("\n         ");
		for(i=0; i<complen; i++)
			printf("%02x ",(unsigned char)OffTable[i]);
		printf("\n\n");
		//printf("%02x   %08x [%02x%02x%02x%02x] %08x [%02x%02x%02x%02x]\n\n",(matchlen+2),(int)buffer,*(unsigned char*)(buffer),*(unsigned char*)(buffer+1),*(unsigned char*)(buffer+2),*(unsigned char*)(buffer+3),(int)buffer-offset,*(unsigned char*)(buffer-offset),*(unsigned char*)(buffer-offset+1),*(unsigned char*)(buffer-offset+2),*(unsigned char*)(buffer-offset+3));
	}
	#endif


	*outlen = matchlen;
	return offset;
}
