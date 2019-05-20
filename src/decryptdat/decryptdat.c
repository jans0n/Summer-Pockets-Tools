#include <stdio.h>
#include <stdlib.h>
#include "compression.h"

//#define KISARAGI // use version 2 instead

int main(int argc, char **argv)
{
	FILE *infile = NULL, *outfile = NULL;
	unsigned char *buffer = NULL, *output = NULL;
	int complen=0, i=0, decomplen=0, size=0;

	if(argc!=3)
	{
		printf("usage: %s infile outfile\n",argv[0]);
		return 0;
	}
	
	infile = fopen(argv[1],"rb");
	if(!infile)
	{
		printf("Could not open %s\n",argv[1]);
		return -1;
	}

	outfile = fopen(argv[2],"wb");

	fseek(infile,0,SEEK_END);
	size = ftell(infile)-8;
	rewind(infile);
	fread(&complen,1,4,infile);
	fread(&decomplen,1,4,infile);

	buffer = (unsigned char*)calloc(size,sizeof(unsigned char));
	fread(buffer,1,size,infile);
	fclose(infile);

#ifdef KISARAGI
	// i don't know what the case is exactly but i'm too lazy to figure it out.
	for(i=0; i<size; i++)
		buffer[i] ^= key2[i&0x0f];
#endif

	if(complen != 0)
	{
		for(i=0; i<complen; i++)
			buffer[i] ^= key2[i&0x0f];
	}
	else
	{
		for(i=0; i<size; i++)
			buffer[i] ^= key[i&0xff];

		complen = *(int*)buffer;
		decomplen = *(int*)(buffer+4);
	}

	output = (unsigned char*)calloc(decomplen, sizeof(unsigned char));
	DecompressData(buffer+8,output,decomplen);

	printf("Decompressed %d bytes\n",decomplen);

	fwrite("\xff\xfe",1,2,outfile); // Unicode BOM
	fwrite(output, 1, decomplen, outfile);

	free(buffer);
	fclose(outfile);

	return 0;
}
