#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <wchar.h>
#include <locale.h>
#include "structs.h"
#include <vector>

using namespace std;
//using std::vector;
//using std::wstring;

FILE *origscript = NULL, *text = NULL, *outfile = NULL;
Entry *stringtable = NULL;
wchar_t *astring = NULL;

int stringLen = 0;

typedef struct _FixLine
{
	int offset;
	int type;
} FixLine;
vector<FixLine*> fixLines;

void ParseText()
{
	int i=0, x=0, j=0, oldlen = 0, b = scrhead.strindex.size, curLine = 0, kk = 0;

	fread(&i,1,2,text);

	if(i!=0xfeff) // skip unicode BOM
		rewind(text);

	scrhead.strindex.size = scrhead.strtable.size = 0;

	while(!feof(text))
	{
		wchar_t temp[2048];

		memset(temp,'\0',2048);
		fgetws(temp,2048,text);

		if(temp[0]=='\0')
			break;

		if(temp[0] == '{')
		{
			unsigned int offset = 0, type = 0;
			int i = 0;

			for(i = 1; i < wcslen(temp); i++)
			{
				if(temp[i] == '}')
					break;

				offset <<= 4;
				if((temp[i] & 0x40) == 0x40)
					offset |= (temp[i] & (~0x20)) - 0x37;
				else if(temp[i] < 0x3a)
					offset |= temp[i] - 0x30;
			}

			if(temp[i] == '}')
				i++;

			while(i < wcslen(temp) && temp[i] == ' ')
				i++;

			for(; i < wcslen(temp); i++)
			{
				if(temp[i] == '\n' || temp[i]=='\r')
					break;
				if((temp[i] < '0' || temp[i] > '9') &&
					(temp[i] < 'A' || temp[i] > 'F') &&
					(temp[i] < 'a' || temp[i] > 'f'))
						break;

				type <<= 4;
				if((temp[i] & 0x40) == 0x40)
					type |= (temp[i] & (~0x20)) - 0x37;
				else if(temp[i] < 0x3a)
					type |= temp[i] - 0x30;
			}

			FixLine *fixme = (FixLine*)calloc(1, sizeof(FixLine));
			fixme->offset = offset;
			fixme->type =  type;
			fixLines.push_back(fixme);

		}
		else if(temp[0]=='<')
		{
			int len = 0, k=0, key = 0;

			if(temp[wcslen(temp)-1]=='\r' || temp[wcslen(temp)-1]=='\n')
				temp[wcslen(temp)-1] = '\0';

			for(k=0; k<wcslen(temp)-1; k++)
			{
				if(temp[k]=='\\' && temp[k+1]=='n')
				{
					int l=0;

					temp[k++] = '\x0a', temp[k] = '\0';

					for(l=k; j<wcslen(temp)-k-2; l++)
						temp[l] = temp[l+1];
				}

				else if(temp[k]=='\\' && temp[k+1]=='\\')
					break;
			}
			temp[k] = '\0';

			for(k=0; k<wcslen(temp)-1; k++)
			{
				if(temp[k]=='>')
				{
					k++;

					while(k < wcslen(temp) - 1 && temp[k] != ' ')
						k++;
					k++;

					break;
				}
			}

			kk = k;
			len = wcslen(temp+kk);
			i++;

			if(scrhead.strtable.size==0)
			{
				astring = (wchar_t*)calloc(len+1,sizeof(wchar_t));
				//astring = (wchar_t*)calloc(0xffffff,sizeof(wchar_t));
				stringtable = (Entry*)calloc(j+1,sizeof(Entry));
			}
			else
			{
				astring = (wchar_t*)realloc(astring,(x+len+1)*2);
				stringtable = (Entry*)realloc(stringtable,(j+1)*sizeof(Entry));
			}

			key = curLine * 0x7087;
			curLine++;
			for(k=0; k<len; k++)
			{
				temp[k+kk] ^= key&0xffff;
			}

			//wcsncat(string,temp+kk,len);
			memcpy(astring+x,temp+kk,len * 2);


			stringtable[j].offset = oldlen;
			stringtable[j].size = len;

			x += len, j++, oldlen += len;
			stringLen = x;
			scrhead.strindex.size += 1;
			scrhead.strtable.size += 1;
		}
	}

	if(scrhead.strindex.size!=b)
	{
		printf("Found %d lines, expected %d\n",scrhead.strindex.size,b);
		exit(1);
	}
}

void InsertScript()
{
	char *buffer = NULL;
	int size = 0;

	fseek(origscript,0,SEEK_END);
	size = ftell(origscript)-scrhead.headersize;
	fseek(origscript,scrhead.headersize,SEEK_SET);

	buffer = (char*)calloc(size,sizeof(char));
	fread(buffer,1,size,origscript);

	scrhead.strindex.offset = size+scrhead.headersize;
	scrhead.strtable.offset = scrhead.strindex.offset+scrhead.strindex.size*sizeof(Entry);

	fwrite(&scrhead,sizeof(scrhead),1,outfile);
	fwrite(buffer,1,size,outfile);
	fwrite(stringtable,sizeof(Entry),scrhead.strindex.size,outfile);
	fwrite(astring,sizeof(wchar_t),stringLen,outfile);
	fwrite("\0\0",1,2,outfile);
}

void InsertNewlines(char *filename)
{
	FILE *infile = fopen(filename, "rb+");

	fread(&scrhead,sizeof(scrhead),1,infile);
	int baseOffset = scrhead.bytecode.offset;

	for(int i = 0; i < fixLines.size(); i++)
	{
		fseek(infile,baseOffset+fixLines[i]->offset,SEEK_SET);
		fwrite(&fixLines[i]->type, 1, 4, infile);
	}

	fclose(infile);
}

void Insert(FILE *file1, FILE *file2, FILE *file3, char *file3Name)
{
	origscript = file1;
	text = file2;
	outfile = file3;

	fread(&scrhead, sizeof(scrhead), 1, origscript);

	ParseText();
	InsertScript();

	fclose(outfile);

	InsertNewlines(file3Name);
}