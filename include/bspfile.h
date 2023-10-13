
#ifndef _BSPFILE_H
#define _BSPFILE_H

// upper design bounds

enum
{
	HULL_POINT = 0,
	HULL_STAND,
	HULL_LARGE,
	HULL_DUCK,
	MAX_MAP_HULLS,
};

#define MAX_MAP_MODELS 512
#define MAX_MAP_BRUSHES 4096
#define MAX_MAP_ENTITIES 1024
#define MAX_MAP_ENTSTRING 131072
#define MAX_MAP_PLANES 32767
#define MAX_MAP_NODES 32767		// because negatives are contents
#define MAX_MAP_CLIPNODES 32767
#define MAX_MAP_LEAFS 8192
#define MAX_MAP_VERTS 65535
#define MAX_MAP_FACES 65535
#define MAX_MAP_MARKSURFACES 65535
#define MAX_MAP_TEXINFO 8192
#define MAX_MAP_EDGES 256000
#define MAX_MAP_SURFEDGES 512000
#define MAX_MAP_TEXTURES 512
#define MAX_MAP_MIPTEX 2097152
#define MAX_MAP_LIGHTING 2097152
#define MAX_MAP_VISIBILITY 2097152

#define MAX_MAP_PORTALS 65536

// key / value pair sizes

#define MAX_KEY 32
#define MAX_VALUE 1024

//=============================================================================

enum
{
	BSPVERSION = 30,
	BSPQUAKE = 29,
};

typedef struct
{
	int32_t fileofs, filelen;
} lump_t;

enum
{
	LUMP_ENTITIES,
	LUMP_PLANES,
	LUMP_TEXTURES,
	LUMP_VERTEXES,
	LUMP_VISIBILITY,
	LUMP_NODES,
	LUMP_TEXINFO,
	LUMP_FACES,
	LUMP_LIGHTING,
	LUMP_CLIPNODES,
	LUMP_LEAFS,
	LUMP_MARKSURFACES,
	LUMP_EDGES,
	LUMP_SURFEDGES,
	LUMP_MODELS,
	HEADER_LUMPS,
};

typedef struct
{
	float mins[3], maxs[3];
	float origin[3];
	int32_t headnode[MAX_MAP_HULLS];
	int32_t visleafs; // not including the solid leaf 0
	int32_t firstface, numfaces;
} dmodel_t;

typedef struct
{
	int32_t version;
	lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
	int32_t nummiptex;
	int32_t dataofs[4]; // [nummiptex]
} dmiptexlump_t;

#define MIPLEVELS 4
typedef struct miptex_s
{
	char name[16];
	uint32_t width, height;
	uint32_t offsets[MIPLEVELS]; // four mip maps stored
} miptex_t;

typedef struct
{
	float point[3];
} dvertex_t;

enum
{
	// 0-2 are axial planes
	PLANE_X,
	PLANE_Y,
	PLANE_Z,
	// 3-5 are non-axial planes snapped to the nearest
	PLANE_ANYX,
	PLANE_ANYY,
	PLANE_ANYZ,
};

typedef struct
{
	float normal[3];
	float dist;
	int32_t type; // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;

enum
{
	CONTENTS_CURRENT_DOWN = -14,
	CONTENTS_CURRENT_UP,
	CONTENTS_CURRENT_270,
	CONTENTS_CURRENT_180,
	CONTENTS_CURRENT_90,
	CONTENTS_CURRENT_0,

	CONTENTS_CLIP,   // changed to contents_solid
	CONTENTS_ORIGIN, // removed at csg time
	CONTENTS_SKY,
	CONTENTS_LAVA,
	CONTENTS_SLIME,
	CONTENTS_WATER,
	CONTENTS_SOLID,
	CONTENTS_EMPTY,
};

typedef struct
{
	int32_t planenum;
	int16_t children[2]; // negative numbers are -(leafs+1), not nodes
	int16_t mins[3];	   // for sphere culling
	int16_t maxs[3];
	uint16_t firstface;
	uint16_t numfaces; // counting both sides
} dnode_t;

typedef struct
{
	int32_t planenum;
	int16_t children[2]; // negative numbers are contents
} dclipnode_t;

typedef struct texinfo_s
{
	float vecs[2][4]; // [s/t][xyz offset]
	int32_t miptex;
	int32_t flags;
} texinfo_t;
#define TEX_SPECIAL 1 // sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	uint16_t v[2]; // vertex numbers
} dedge_t;

#define MAXLIGHTMAPS 4
typedef struct
{
	int16_t planenum;
	int16_t side;

	int32_t firstedge; // we must support > 64k edges
	int16_t numedges;
	int16_t texinfo;

	// lighting info
	byte styles[MAXLIGHTMAPS];
	int32_t lightofs; // start of [numstyles*surfsize] samples
} dface_t;

enum
{
	AMBIENT_WATER = 0,
	AMBIENT_SKY,
	AMBIENT_SLIME,
	AMBIENT_LAVA,
	NUM_AMBIENTS, // automatic ambient sounds
};

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int32_t contents;
	int32_t visofs; // -1 = no visibility info

	int16_t mins[3]; // for frustum culling
	int16_t maxs[3];

	uint16_t firstmarksurface;
	uint16_t nummarksurfaces;

	byte ambient_level[NUM_AMBIENTS];
} dleaf_t;

#endif /* !_BSPFILE_H */
