
#ifndef _SPRITEGN_H
#define _SPRITEGN_H

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dsprite_t file header structure
// <repeat dsprite_t.numframes times>
//   <if spritegroup, repeat dspritegroup_t.numframes times>
//     dspriteframe_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dspriteframe_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#define SPRITE_VERSION 1

// TODO: shorten these?
typedef struct
{
	int32_t ident;
	int32_t version;
	int32_t type;
	float boundingradius;
	int32_t width;
	int32_t height;
	int32_t numframes;
	float beamlength;
	synctype_t synctype;
} dsprite_t;

enum
{
	SPR_VP_PARALLEL_UPRIGHT = 0,
	SPR_FACING_UPRIGHT,
	SPR_VP_PARALLEL,
	SPR_ORIENTED,
	SPR_VP_PARALLEL_ORIENTED,
};

typedef struct
{
	int32_t origin[2];
	int32_t width;
	int32_t height;
} dspriteframe_t;

typedef struct
{
	int32_t numframes;
} dspritegroup_t;

typedef struct
{
	float interval;
} dspriteinterval_t;

typedef enum
{
	SPR_SINGLE = 0,
	SPR_GROUP
} spriteframetype_t;

typedef struct
{
	spriteframetype_t type;
} dspriteframetype_t;

#define IDSPRITEHEADER (('P' << 24) + ('S' << 16) + ('D' << 8) + 'I') // little-endian "IDSP"

#endif /* !_SPRITEGN_H */
