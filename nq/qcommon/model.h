
#ifndef _MODEL_H
#define _MODEL_H

#include "bspfile.h"
#include "modelgen.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/
typedef struct efrag_s efrag_t;

//
// in memory representation
//
typedef struct
{
	vec3_t position;
} mvertex_t;

enum
{
	SIDE_FRONT = 0,
	SIDE_BACK,
	SIDE_ON,
};

// plane_t structure
typedef struct mplane_s
{
	vec3_t normal;
	float dist;
	byte type;	   // for texture axis selection and fast side tests
	byte signbits; // signx + signy<<1 + signz<<1
	byte pad[2];
} mplane_t;

typedef struct texture_s
{
	char name[16];
	unsigned width, height;
	int gl_texturenum;
	int gl_brightnum;
	struct msurface_s *texturechain;   // for gl_texsort drawing
	int anim_total;					   // total tenths in sequence ( 0 = no)
	int anim_min, anim_max;			   // time for this frame min <=time< max
	struct texture_s *anim_next;	   // in the animation sequence
	struct texture_s *alternate_anims; // bmodels in frmae 1 use these
	unsigned offsets[MIPLEVELS];	   // four mip maps stored
} texture_t;

enum
{
	SURF_PLANEBACK		= 1 << 1,
	SURF_DRAWSKY		= 1 << 2,
	SURF_DRAWSPRITE		= 1 << 3,
	SURF_DRAWTURB		= 1 << 4,
	SURF_DRAWTILED		= 1 << 5,
	SURF_DRAWBACKGROUND	= 1 << 6,
	SURF_UNDERWATER		= 1 << 7,
	SURF_DRAWFENCE		= 1 << 8,
	SURF_NODRAW			= 1 << 9,
};

typedef struct
{
	unsigned short v[2];
	unsigned int cachededgeoffset;
} medge_t;

typedef struct
{
	float vecs[2][4];
	float mipadjust;
	texture_t *texture;
	int flags;
} mtexinfo_t;

#define VERTEXSIZE 7

typedef struct glpoly_s
{
	struct glpoly_s *next;
	struct glpoly_s *chain;
	int numverts;
	int flags;					// for SURF_UNDERWATER
	float verts[4][VERTEXSIZE]; // variable sized (xyz s1t1 s2t2)
} glpoly_t;

typedef struct msurface_s
{
	int visframe; // should be drawn when node is crossed

	mplane_t *plane;
	int flags;

	int firstedge; // look up in model->surfedges[], negative numbers
	int numedges;  // are backwards edges

	short texturemins[2];
	short extents[2];

	int light_s, light_t; // gl lightmap coordinates

	glpoly_t *polys; // multiple if warped
	struct msurface_s *texturechain;

	mtexinfo_t *texinfo;

	// lighting info
	int dlightframe;
	int dlightbits;

	int lightmaptexturenum;
	byte styles[MAXLIGHTMAPS];
	int cached_light[MAXLIGHTMAPS]; // values currently used in lightmap
	bool cached_dlight;				// true if dynamic light in cache
	byte *samples;					// [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
	// common with leaf
	int contents; // 0, to differentiate from leafs
	int visframe; // node needs to be traversed if current

	float minmaxs[6]; // for bounding box culling

	struct mnode_s *parent;

	// node specific
	mplane_t *plane;
	struct mnode_s *children[2];

	unsigned short firstsurface;
	unsigned short numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
	// common with node
	int contents; // wil be a negative contents number
	int visframe; // node needs to be traversed if current

	float minmaxs[6]; // for bounding box culling

	struct mnode_s *parent;

	// leaf specific
	byte *compressed_vis;
	efrag_t *efrags;

	msurface_t **firstmarksurface;
	int nummarksurfaces;
	int key; // BSP sequence number for leaf's contents
	byte ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

typedef struct
{
	dclipnode_t *clipnodes;
	mplane_t *planes;
	int firstclipnode;
	int lastclipnode;
	vec3_t clip_mins;
	vec3_t clip_maxs;
} hull_t;

extern const vec3_t hull_sizes[MAX_MAP_HULLS][2];
extern const vec3_t quake_hull_sizes[MAX_MAP_HULLS][2];

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

// FIXME: shorten these?
typedef struct mspriteframe_s
{
	int width;
	int height;
	float up, down, left, right;
	int gl_texturenum;
} mspriteframe_t;

typedef struct
{
	int numframes;
	float *intervals;
	mspriteframe_t *frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t type;
	mspriteframe_t *frameptr;
} mspriteframedesc_t;

typedef struct
{
	int type;
	int maxwidth;
	int maxheight;
	int numframes;
	float beamlength; // remove?
	void *cachespot;  // remove?
	mspriteframedesc_t frames[1];
} msprite_t;

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

#define ALIAS_BASE_SIZE_RATIO (1.0 / 11.0)

// normalizing factor so player model works out to about
//  1 pixel per triangle
#define MAX_LBM_HEIGHT 480

typedef struct
{
	int firstpose;
	int numposes;
	float interval;
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
	char name[16];
} maliasframedesc_t;

typedef struct
{
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
} maliasgroupframedesc_t;

typedef struct
{
	int numframes;
	int intervals;
	maliasgroupframedesc_t frames[1];
} maliasgroup_t;

typedef struct mtriangle_s
{
	int facesfront;
	int vertindex[3];
} mtriangle_t;

#define MAX_SKINS 32
typedef struct
{
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;

	int numposes;
	int poseverts;
	int posedata; // numposes*poseverts trivert_t
	int commands; // gl command list with embedded s/t
	int gl_texturenum[MAX_SKINS][4];
	int gl_brightnum[MAX_SKINS][4];
	int texels[MAX_SKINS];		 // only for player skins
	maliasframedesc_t frames[1]; // variable sized
} aliashdr_t;

#define MAXALIASVERTS 1024
#define MAXALIASFRAMES 256
#define MAXALIASTRIS 2048

extern aliashdr_t *pheader;
extern stvert_t stverts[MAXALIASVERTS];
extern mtriangle_t triangles[MAXALIASTRIS];
extern trivertx_t *poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias
} modtype_t;

enum
{
	EF_ROCKET	 = 1 << 0, // leave a trail
	EF_GRENADE	 = 1 << 1, // leave a trail
	EF_GIB		 = 1 << 2, // leave a trail
	EF_ROTATE	 = 1 << 3, // rotate (bonus items)
	EF_TRACER	 = 1 << 4, // green split trail
	EF_ZOMGIB	 = 1 << 5, // small blood trail
	EF_TRACER2	 = 1 << 6, // orange split trail + rotate
	EF_TRACER3	 = 1 << 7, // purple trail
};

typedef struct model_s
{
	char name[MAX_QPATH];
	bool needload; // bmodels and sprites don't cache normally

	modtype_t type;
	int numframes;
	synctype_t synctype;

	int flags;

	//
	// volume occupied by the model graphics
	//
	vec3_t mins, maxs;
	float radius;

	//
	// solid volume for clipping
	//
	bool clipbox;
	vec3_t clipmins, clipmaxs;

	//
	// brush model
	//
	bool world;

	int firstmodelsurface, nummodelsurfaces;

	int numsubmodels;
	dmodel_t *submodels;

	int numplanes;
	mplane_t *planes;

	int numleafs; // number of visible leafs, not counting 0
	mleaf_t *leafs;

	int numvertexes;
	mvertex_t *vertexes;

	int numedges;
	medge_t *edges;

	int numnodes;
	mnode_t *nodes;

	int numtexinfo;
	mtexinfo_t *texinfo;

	int numsurfaces;
	msurface_t *surfaces;

	int numsurfedges;
	int *surfedges;

	int numclipnodes;
	dclipnode_t *clipnodes;

	int nummarksurfaces;
	msurface_t **marksurfaces;

	hull_t hulls[MAX_MAP_HULLS];

	int numtextures;
	texture_t **textures;

	byte *visdata;
	byte *lightdata;
	char *entities;

	//
	// additional model data
	//
	cache_user_t cache; // only access through Mod_Extradata
} model_t;

//============================================================================

extern texture_t *r_notexture_mip;

void Mod_Init();
void Mod_ClearAll();
model_t *Mod_ForName(char *name, bool crash, bool world);
void *Mod_Extradata(model_t *mod); // handles caching
void Mod_TouchModel(char *name);

mleaf_t *Mod_PointInLeaf(float *p, model_t *model);
byte *Mod_LeafPVS(mleaf_t *leaf, model_t *model);

#endif /* !_MODEL_H */
