#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <clocale>
#include "structs.h"
#include <vector>
#include "ssinsert.h"


using namespace std;

typedef struct _NewlineAssociation
{
	int line;
	int offset;
	int type;
} NewlineAssociation;

Scrhead scrhead;

void Dump(FILE *infile, FILE *outfile)
{
	vector<NewlineAssociation*> newlines;
	Entry *strings = NULL;
	unsigned char *bytecode = NULL;
	int i = 0;

	fread(&scrhead,sizeof(scrhead),1,infile);
	fwrite("\xff\xfe",1,2,outfile);

	bytecode = (unsigned char*)calloc(scrhead.bytecode.size,sizeof(unsigned char));
	fseek(infile,scrhead.bytecode.offset,SEEK_SET);
	fread(bytecode,1,scrhead.bytecode.size,infile);

	fwprintf(outfile,L"// Newline command format: {0000} (newline type)\r\n// 0d = append\r\n// 0e = new page\r\n// anything else is unknown\r\n\r\n");

	for(i = 0; i < scrhead.bytecode.size; i++)
	{
		static unsigned char pushString[] = { 0x02, 0x14, 0x00, 0x00, 0x00 };
		static unsigned char pushNewline[] = { 0x02, 0x0a, 0x00, 0x00, 0x00 };
		static unsigned char pushNewline2[] = { 0x30, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 03, 00, 00, 00, 00 };
		int x = 0;

		if(memcmp(bytecode+i, pushString, 5) == 0)
		{
			unsigned int stringNum = *(unsigned int*)(bytecode+i+5);
			unsigned int pushType = 0;
			int foundPush = 0;

			for(x = i + 5; x < scrhead.bytecode.size - 5; x++)
			{
				if(x - i > 0x100)
					break;

				if(memcmp(bytecode+x, pushString, 5) == 0)
					break;

				if(memcmp(bytecode+x, pushNewline, 5) == 0 && memcmp(bytecode+x+9, pushNewline2, 0x16) == 0)
				{
					foundPush = 1;
					pushType = *(unsigned int*)(bytecode+x+5);
					break;
				}
			}

			if(foundPush && pushType != 0x0e)
			{
				//printf("Found match at %08x (#%d) (%02x) // line %d\n",scrhead.bytecode.offset+i,stringNum, pushType, stringNum);
				//fwprintf(outfile,L"{%04x} %02x // line <%04d>\r\n", x+5, pushType, stringNum);

				NewlineAssociation *newline = (NewlineAssociation*)calloc(1, sizeof(NewlineAssociation));
				newline->line = stringNum;
				newline->offset = x+5;
				newline->type = pushType;
				newlines.push_back(newline);
			}
			else
			{
				//printf("Could not find push\n");
			}
			//exit(1);
		}
	}

	strings = (Entry*)calloc(scrhead.strindex.size,sizeof(Entry));
	fseek(infile,scrhead.strindex.offset,SEEK_SET);
	fread(strings,sizeof(Entry),scrhead.strindex.size,infile);

	for(i=0; i<scrhead.strindex.size; i++)
	{
		wchar_t *line = (wchar_t*)calloc(strings[i].size+1,sizeof(wchar_t));
		wchar_t *lineparsed = (wchar_t*)calloc(strings[i].size*2,sizeof(wchar_t));
		int x = 0, j = 0, l = 0;
		int key = 0;

		fseek(infile,scrhead.strtable.offset+strings[i].offset*sizeof(wchar_t),SEEK_SET);
		fread(line,sizeof(wchar_t),strings[i].size,infile);

		// for v2.00 of rewrite trial (and probably newer siglusengine stuff)
		key = i * 0x7087;
		for(l=0; l<strings[i].size; l++)
		{
			line[l] ^= key;
		}

		fwprintf(outfile,L"//");
		for(l=0; l<2; l++)
		{
			fwprintf(outfile,L"<%04d> ",i);

			for(x=0,j=0; x<strings[i].size; j++, x++)
			{
				if(line[x]=='\n')
					fwprintf(outfile,L"\\n");
				else
					fwprintf(outfile,L"%c",line[x]);
			}

			fwprintf(outfile,L"\r\n");
		}

		for(x = 0; x < newlines.size(); x++)
		{
			if(newlines[x]->line == i)
			{
				//printf("Found match: {%04x} %02x // line <%04d>\n", newlines[x]->offset, newlines[x]->type, newlines[x]->line);
				fwprintf(outfile,L"{%04x} %02x // %04d\r\n", newlines[x]->offset, newlines[x]->type, newlines[x]->line);
				break;
			}
		}
		fwprintf(outfile,L"\r\n");

		free(line);
	}

	free(strings);
	free(bytecode);
}


void verifyFile(FILE *file, char *fileName)
{
	if (!file)
	{
		printf("Could not open %s\n", fileName);
		exit(-1);
	}
}


int main(int argc, char **argv)
{
	FILE *file1 = NULL, *file2 = NULL, *file3 = NULL;

	if(argc<4 || argc>5)
	{
		printf("usage: %s dump <inscript> <outtext>\n", argv[0]);
		printf("usage: %s insert <inscript> <intext> <outscript>\n", argv[0]);
		return 0;
	}

	char *mode = argv[1];

	if (strcmp(mode,"dump") != 0 && strcmp(mode,"insert") != 0)
	{
		printf("Invalid mode '%s'\n", mode);
		return 0;
	}

	setlocale(LC_ALL, "Japanese");

	file1 = fopen(argv[2],"rb");
	verifyFile(file1, argv[2]);

	// Dump
	if (strcmp(mode,"dump") == 0)
	{
		file2 = fopen(argv[3],"wb");
		Dump(file1, file2);
	}
	// Insert
	else
	{
		file2 = fopen(argv[3],"rb");
		verifyFile(file2, argv[3]);

		file3 = fopen(argv[4], "wb");
		Insert(file1, file2, file3, argv[4]);
	}

	return 0;
}
