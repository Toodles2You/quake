
#ifndef _BSPLIB_H
#define _BSPLIB_H

#include "bspfile.h"

extern int			nummodels;
extern dmodel_t	dmodels[MAX_MAP_MODELS];

extern int			visdatasize;
extern byte		dvisdata[MAX_MAP_VISIBILITY];

extern int			lightdatasize;
extern byte		dlightdata[MAX_MAP_LIGHTING];

extern int			texdatasize;
extern byte		dtexdata[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

extern int			entdatasize;
extern char		dentdata[MAX_MAP_ENTSTRING];

extern int			numleafs;
extern dleaf_t		dleafs[MAX_MAP_LEAFS];

extern int			numplanes;
extern dplane_t	dplanes[MAX_MAP_PLANES];

extern int			numvertexes;
extern dvertex_t	dvertexes[MAX_MAP_VERTS];

extern int			numnodes;
extern dnode_t		dnodes[MAX_MAP_NODES];

extern int			numtexinfo;
extern texinfo_t	texinfo[MAX_MAP_TEXINFO];

extern int			numfaces;
extern dface_t		dfaces[MAX_MAP_FACES];

extern int			numclipnodes;
extern dclipnode_t	dclipnodes[MAX_MAP_CLIPNODES];

extern int			numedges;
extern dedge_t		dedges[MAX_MAP_EDGES];

extern int			nummarksurfaces;
extern unsigned short		dmarksurfaces[MAX_MAP_MARKSURFACES];

extern int			numsurfedges;
extern int			dsurfedges[MAX_MAP_SURFEDGES];

#endif /* !_BSPLIB_H */
