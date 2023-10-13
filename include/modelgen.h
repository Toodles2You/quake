
#ifndef _MODELGEN_H
#define _MODELGEN_H

#define ALIAS_VERSION 6

#define ALIAS_ONSEAM 0x0020

typedef enum
{
	ST_SYNC = 0,
	ST_RAND
} synctype_t;

typedef enum
{
	ALIAS_SINGLE = 0,
	ALIAS_GROUP
} aliasframetype_t;

typedef enum
{
	ALIAS_SKIN_SINGLE = 0,
	ALIAS_SKIN_GROUP
} aliasskintype_t;

typedef struct
{
	int32_t ident;
	int32_t version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int32_t numskins;
	int32_t skinwidth;
	int32_t skinheight;
	int32_t numverts;
	int32_t numtris;
	int32_t numframes;
	synctype_t synctype;
	int32_t flags;
	float size;
} mdl_t;

// TODO: could be shorts

typedef struct
{
	int32_t onseam;
	int32_t s;
	int32_t t;
} stvert_t;

typedef struct dtriangle_s
{
	int32_t facesfront;
	int32_t vertindex[3];
} dtriangle_t;

#define DT_FACES_FRONT 0x0010

// This mirrors trivert_t in trilib.h, is present so Quake knows how to
// load this data

typedef struct
{
	byte v[3];
	byte lightnormalindex;
} trivertx_t;

typedef struct
{
	trivertx_t bboxmin; // lightnormal isn't used
	trivertx_t bboxmax; // lightnormal isn't used
	char name[16];		// frame name from grabbing
} daliasframe_t;

typedef struct
{
	int32_t numframes;
	trivertx_t bboxmin; // lightnormal isn't used
	trivertx_t bboxmax; // lightnormal isn't used
} daliasgroup_t;

typedef struct
{
	int32_t numskins;
} daliasskingroup_t;

typedef struct
{
	float interval;
} daliasinterval_t;

typedef struct
{
	float interval;
} daliasskininterval_t;

typedef struct
{
	aliasframetype_t type;
} daliasframetype_t;

typedef struct
{
	aliasskintype_t type;
} daliasskintype_t;

#define IDPOLYHEADER (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I') // little-endian "IDPO"

#endif /* !_MODELGEN_H */
