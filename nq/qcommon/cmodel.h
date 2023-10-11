
#ifndef _CMODEL_H
#define _CMODEL_H

#include "bspfile.h"
#include "modelgen.h"
#include "spritegn.h"

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias
} modtype_t;

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

typedef struct cnode_s cnode_t;

typedef struct cnode_s
{
	// common with leaf
	int contents; // 0, to differentiate from leafs
	int visframe; // node needs to be traversed if current

	float minmaxs[6]; // for bounding box culling

	cnode_t *parent;

	// node specific
	mplane_t *plane;
	cnode_t *children[2];
} cnode_t;

typedef struct cleaf_s
{
	// common with node
	int contents; // wil be a negative contents number
	int visframe; // node needs to be traversed if current

	float minmaxs[6]; // for bounding box culling

	cnode_t *parent;

	// leaf specific
	byte *compressed_vis;
} cleaf_t;

typedef struct
{
	dclipnode_t *clipnodes;
	mplane_t *planes;
	int firstclipnode;
	int lastclipnode;
	vec3_t clip_mins;
	vec3_t clip_maxs;
} hull_t;

typedef struct cmodel_s
{
	char name[MAX_QPATH];
	bool needload;

	modtype_t type;

	vec3_t mins, maxs;

	bool world;

	int numsubmodels;
	dmodel_t *submodels;

	int numplanes;
	mplane_t *planes;

	int numleafs; // number of visible leafs, not counting 0
	cleaf_t *leafs;

	int numnodes;
	cnode_t *nodes;

	int numclipnodes;
	dclipnode_t *clipnodes;

	hull_t hulls[MAX_MAP_HULLS];

	byte *visdata;
	char *entities;
} cmodel_t;

extern const vec3_t hull_sizes[MAX_MAP_HULLS][2];
extern const vec3_t quake_hull_sizes[MAX_MAP_HULLS][2];

//============================================================================

void CMod_Init();
void CMod_ClearAll();
cmodel_t *CMod_ForName(char *name, bool crash, bool world);

cleaf_t *CMod_PointInLeaf(vec3_t p, cmodel_t *model);
byte *CMod_LeafPVS(cleaf_t *leaf, cmodel_t *model);

#endif /* !_CMODEL_H */
