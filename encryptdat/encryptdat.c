#include <stdio.h>
#include <stdlib.h>
#include "compression.c"


void Decompress(FILE *infile, FILE *outfile)
{
	unsigned char *buffer = NULL, *output = NULL;
	int complen=0, i=0, decomplen=0, size=0;

	fseek(infile,0,SEEK_END);
	size = ftell(infile)-8;
	rewind(infile);
	fread(&complen,1,4,infile);
	fread(&decomplen,1,4,infile);

	buffer = (unsigned char*)calloc(size,sizeof(unsigned char));
	fread(buffer,1,size,infile);
	fclose(infile);

	if(complen != size)
	{
		printf("complen:%08x\n",complen);
		for(i=0; i<size; i++)
			buffer[i] ^= key2[i&0x0f];

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
}

void Compress(FILE *infile, FILE *outfile)
{
	unsigned char *buffer = NULL, *output = NULL;
	int complen=0, i=0, size=0;

	fseek(infile,0,SEEK_END);
	size = ftell(infile);
	rewind(infile);

	fread(&i,1,2,infile);

	if(i!=0xfeff)
		rewind(infile);
	else
		size -= 2;

	buffer = (unsigned char*)calloc(size,sizeof(unsigned char));
	fread(buffer,1,size,infile);
	fclose(infile);

	output = CompressData(buffer,size,&complen,17);

	printf("Compressed %d bytes into %d bytes\n",size,complen);

	for(i=0; i<complen; i++)
		output[i] ^= key[i&0xff];

	fwrite("\x00\x00\x00\x00",1,4,outfile);
	fwrite("\x00\x00\x00\x00",1,4,outfile);
	fwrite(output, 1, complen, outfile);
	free(buffer);
	free(output);
	fclose(outfile);
}


int main(int argc, char **argv)
{
	FILE *infile = NULL, *outfile = NULL;

	if(argc!=4)
	{
		printf("usage: %s [mode] infile outfile\n",argv[0]);
		printf("  modes:\n");
		printf("   encrypt     encrypts the gameexe.dat\n");
		printf("   decrypt     decrypts the gameexe.dat\n");
		return 0;
	}

	if (strcmp(argv[1],"encrypt") > 0 && strcmp(argv[1],"decrypt") > 0)
	{
		printf("Invalid mode '%s'\n", argv[1]);
		return 0;
	}

	infile = fopen(argv[2],"rb");
	if(!infile)
	{
		printf("Could not open %s\n",argv[2]);
		return -1;
	}

	outfile = fopen(argv[3],"wb");

	if (strcmp(argv[1],"encrypt") == 0)
	{
		Compress(infile, outfile);
	}
	else
	{
		Decompress(infile, outfile);
	}

	return 0;
}
