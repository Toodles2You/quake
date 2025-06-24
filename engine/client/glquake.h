
#ifndef _GLQUAKE_H
#define _GLQUAKE_H

#include <GL/gl.h>
#include <GL/glu.h>

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

extern int texture_extension_number;
extern int gl_filter_min;
extern int gl_filter_max;

void GL_LoadTexture (int *gl_texturenum, int *gl_brightnum, char *identifier, int width, int height, byte *data, int bytes, int colors, byte *pal, bool mipmap,
					 bool alpha);

extern int glx, gly, glwidth, glheight;

#define BACKFACE_EPSILON 0.01f

typedef enum
{
	pt_static,
	pt_grav,
	pt_slowgrav,
	pt_fire,
	pt_explode,
	pt_explode2,
	pt_blob,
	pt_blob2
} ptype_t;

typedef struct particle_s
{
	// driver-usable fields
	vec3_t org;
	float color;
	// drivers never touch the following fields
	struct particle_s *next;
	vec3_t vel;
	float ramp;
	float die;
	ptype_t type;
} particle_t;

extern entity_t r_worldentity;
extern vec3_t modelorg;
extern entity_t *currententity;
extern int r_visframecount; // ??? what difs?
extern int r_framecount;
extern mplane_t frustum[4];
extern int c_brush_polys, c_alias_polys;

//
// view origin
//
extern vec3_t vup;
extern vec3_t vpn;
extern vec3_t vright;
extern vec3_t r_origin;

//
// screen size info
//
extern refdef_t r_refdef;
extern mleaf_t *r_viewleaf, *r_oldviewleaf;
extern int d_lightstylevalue[256]; // 8.8 fraction of base light value

extern int currenttexture;
extern int particletexture;
extern int netgraphtexture;

extern cvar_t r_norefresh;
extern cvar_t r_drawentities;
extern cvar_t r_drawworld;
extern cvar_t r_drawviewmodel;
extern cvar_t r_speeds;
extern cvar_t r_waterwarp;
extern cvar_t r_fullbright;
extern cvar_t r_lightmap;
extern cvar_t r_wateralpha;
extern cvar_t r_dynamic;
extern cvar_t r_novis;
extern cvar_t r_netgraph;
extern cvar_t r_fence;
extern cvar_t r_luminescent;
extern cvar_t r_zmax;
extern cvar_t r_wireframe;

extern cvar_t gl_clear;
extern cvar_t gl_poly;
extern cvar_t gl_texsort;
extern cvar_t gl_smoothmodels;
extern cvar_t gl_affinemodels;
extern cvar_t gl_polyblend;
extern cvar_t gl_keeptjunctions;
extern cvar_t gl_partblend;

extern cvar_t gl_max_size;
extern cvar_t gl_playermip;

extern float r_world_matrix[16];

void GL_Bind (int texnum);

// Multitexture
#define TEXTURE0_SGIS 0x835E
#define TEXTURE1_SGIS 0x835F

#ifndef APIENTRY
#define APIENTRY
#endif

typedef void (APIENTRY *lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC) (GLenum);
extern lpMTexFUNC qglMTexCoord2fSGIS;
extern lpSelTexFUNC qglSelectTextureSGIS;

extern bool gl_mtexable;

void GL_DisableMultitexture (void);
void GL_EnableMultitexture (void);

// gl_warp.c
void GL_SubdivideSurface (mbrush_t *model, msurface_t *fa);
void EmitBothSkyLayers (msurface_t *fa);
void EmitWaterPolys (msurface_t *fa);
void EmitSkyPolys (msurface_t *fa);
void R_DrawSkyChain (msurface_t *s);
void R_DrawSkyBox (void);
void R_ClearSkyBox (void);

// gl_draw.c
void GL_Set2D (void);

// gl_rmain.c
bool R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (entity_t *e);

// gl_rlight.c
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void R_AnimateLight (void);
void R_LightPoint (vec3_t p, vec3_t dest);

// gl_refrag.c
void R_StoreEfrags (efrag_t **ppefrag);

// gl_mesh.c
void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);

// gl_rsurf.c
void R_DrawWaterSurfaces (void);
void R_DrawBrushModel (entity_t *e);
void R_DrawWorld (void);
void GL_BuildLightmaps (void);

// gl_ngraph.c
void R_NetGraph (void);

#endif /* !_GLQUAKE_H */
