#pragma pack(1)
#ifndef _structs_h_
#define _structs_h_

typedef struct
{
	int offset;
	int size;
} Entry;

typedef struct
{
	unsigned char *buffer;
	int size;
} EntryChar;

typedef struct
{
	wchar_t *buffer;
	int size;
} EntryStr;

typedef struct
{
	char *name;
	int nameoffset;
	int namesize;
	int offset;
	int size;
} FileEntry;

typedef struct
{
	unsigned char *stack;
	int size;
} Stack;

typedef struct
{
	int *stack;
	int size;
} StackInt;

struct Scrhead
{
	int headersize; // 0x00
	Entry bytecode; // 0x04
	Entry strindex; // 0x0c
	Entry strtable; // 0x14
	Entry labels; // 0x1c
	Entry markers; // 0x24
	Entry unk3; // 0x2c // some kind of label. possibly functions or variables of some kind?
	Entry unk4; // 0x34 // variable type table?
	Entry unk5; // 0x3c // string index
	Entry unk6; // 0x44 // string table
	Entry unk7; // 0x4c // corresponds with the stuff in unk3. only offsets
	Entry unk8; // 0x54 // string index
	Entry unk9; // 0x5c // string table
	Entry unk10; // 0x64 // string index
	Entry unk11; // 0x6c // string table
	Entry unk12; // 0x74
	Entry unk13; // 0x7c
};

extern Scrhead scrhead;

struct Pckhead
{
	int headersize;
	EntryChar table1;
	EntryChar globalvartable;
	EntryStr globalvartablestr;
	EntryChar filenametable1;
	EntryChar filenametable2;
	EntryChar filenametable3;
	EntryChar filenametable4;
	EntryStr filenamestr;
	EntryChar filetable;
	EntryChar filestart;
	//EntryChar unk2;
	int extraencrypt;
	int unk2;
};

extern Pckhead pckhead;
#endif

#pragma pack()
