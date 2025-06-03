
#ifndef _WAD_H
#define _WAD_H

enum
{
	CMP_NONE = 0,
	CMP_LZSS,
};

enum
{
	TYP_NONE = 0,
	TYP_LABEL,

	TYP_LUMPY = 64, // 64 + grab command number
	TYP_PALETTE,
	TYP_QTEX,
	TYP_QPIC,
	TYP_SOUND,
	TYP_MIPTEX,
};

typedef struct
{
	int width, height;
	byte data[4]; // variably sized
} qpic_t;

typedef struct
{
	char identification[4]; // should be WAD2 or 2DAW
	int numlumps;
	int infotableofs;
} wadinfo_t;

typedef struct
{
	int filepos;
	int disksize;
	int size; // uncompressed
	char type;
	char compression;
	char pad1, pad2;
	char name[16]; // must be null terminated
} lumpinfo_t;

#endif /* !_WAD_H */
