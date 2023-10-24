/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023 Justin Keller

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
===========================================================================
*/

#include "bothdef.h"

const vec3_t hull_sizes[MAX_MAP_HULLS][2] =
{
    {{0, 0, 0}, {0, 0, 0}},
    {{-16, -16, -36}, {16, 16, 36}},
    {{-32, -32, -36}, {32, 32, 36}},
    {{-16, -16, -18}, {16, 16, 18}},
};

const vec3_t quake_hull_sizes[MAX_MAP_HULLS][2] =
{
    {{0, 0, 0}, {0, 0, 0}},
    {{-16, -16, -24}, {16, 16, 32}},
    {{-32, -32, -24}, {32, 32, 64}},
    {{-16, -16, -12}, {16, 16, 16}},
};

void CMod_LoadBrushModel(cmodel_t *mod, void *buffer);

static cmodel_t *loadcmod;
static char     loadname[32]; // for hunk tags
static byte     cmod_novis[MAX_MAP_LEAFS / 8];
static cmodel_t cmod_known[MAX_MODELS];
static int      cmod_numknown;

/*
===============
CMod_Init
===============
*/
void CMod_Init()
{
    memset(cmod_novis, 255, sizeof(cmod_novis));
}

/*
===============
CMod_PointInLeaf
===============
*/
mleaf_t *CMod_PointInLeaf(vec3_t p, cmodel_t *model)
{
    mnode_t *node;
    float d;
    mplane_t *plane;

    node = model->nodes;
    while (true)
    {
        if (node->contents < 0)
        {
            return (mleaf_t *)node;
        }
        plane = node->plane;
        d = DotProduct(p, plane->normal) - plane->dist;
        if (d > 0)
        {
            node = node->children[0];
        }
        else
        {
            node = node->children[1];
        }
    }

    return NULL; // never reached
}

/*
===================
CMod_DecompressVis
===================
*/
byte *CMod_DecompressVis(byte *in, cmodel_t *model)
{
    static byte decompressed[MAX_MAP_LEAFS / 8];
    int c;
    byte *out;
    int row;

    row = (model->numleafs + 7) >> 3;
    out = decompressed;

    if (!in)
    { // no vis info, so make all visible
        while (row)
        {
            *out++ = 0xff;
            row--;
        }
        return decompressed;
    }

    do
    {
        if (*in)
        {
            *out++ = *in++;
            continue;
        }

        c = in[1];
        in += 2;
        while (c)
        {
            *out++ = 0;
            c--;
        }
    } while (out - decompressed < row);

    return decompressed;
}

byte *CMod_LeafPVS(mleaf_t *leaf, cmodel_t *model)
{
    if (leaf == model->leafs)
    {
        return cmod_novis;
    }
    return CMod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
CMod_ClearAll
===================
*/
void CMod_ClearAll()
{
    int i;
    cmodel_t *mod;

    for (i = 0, mod = cmod_known; i < cmod_numknown; i++, mod++)
    {
        mod->needload = true;
    }
}

/*
==================
CMod_FindName
==================
*/
cmodel_t *CMod_FindName(char *name)
{
    int i;
    cmodel_t *mod;

    if (!name[0])
    {
        Sys_Error("CMod_ForName: NULL name");
    }

    //
    // search the currently loaded models
    //
    for (i = 0, mod = cmod_known; i < cmod_numknown; i++, mod++)
    {
        if (!strcmp(mod->name, name))
        {
            break;
        }
    }

    if (i == cmod_numknown)
    {
        if (cmod_numknown == MAX_MODELS)
        {
            Sys_Error("cmod_numknown == MAX_MODELS");
        }
        strcpy(mod->name, name);
        mod->needload = true;
        cmod_numknown++;
    }

    return mod;
}

/*
==================
CMod_LoadModel

Loads a model into the cache
==================
*/
cmodel_t *CMod_LoadModel(cmodel_t *mod, bool crash, bool world)
{
    uint32_t *buf;
    byte stackbuf[1024]; // avoid dirtying the cache heap

    if (!mod->needload)
    {
        return mod; // not cached at all
    }

    //
    // load the file
    //
    buf = (uint32_t *)COM_LoadStackFile(mod->name, stackbuf, sizeof(stackbuf));
    if (!buf)
    {
        if (crash)
        {
            Sys_Error("CMod_NumForName: %s not found", mod->name);
        }
        return NULL;
    }

    //
    // allocate a new model
    //
    COM_FileBase(mod->name, loadname);

    loadcmod = mod;

    //
    // fill it in
    //

    // call the apropriate loader
    mod->needload = false;
    mod->world = world;

    switch (LittleLong(*(uint32_t *)buf))
    {
    case IDPOLYHEADER:
        mod->type = mod_alias;
        break;

    case IDSPRITEHEADER:
        mod->type = mod_sprite;
        break;

    default:
        mod->type = mod_brush;
        CMod_LoadBrushModel(mod, buf);
        break;
    }

    return mod;
}

/*
==================
CMod_ForName

Loads in a model for the given name
==================
*/
cmodel_t *CMod_ForName(char *name, bool crash, bool world)
{
    cmodel_t *mod = CMod_FindName(name);

    return CMod_LoadModel(mod, crash, world);
}

/*
===============================================================================

                    BRUSHMODEL LOADING

===============================================================================
*/

static byte *mod_base;
static int mod_version;

/*
=================
CMod_LoadVisibility
=================
*/
void CMod_LoadVisibility(lump_t *l)
{
    if (!l->filelen)
    {
        loadcmod->visdata = NULL;
        return;
    }
    loadcmod->visdata = Hunk_AllocName(l->filelen, loadname);
    memcpy(loadcmod->visdata, mod_base + l->fileofs, l->filelen);
}

/*
=================
CMod_LoadEntities
=================
*/
void CMod_LoadEntities(lump_t *l)
{
    if (!loadcmod->world || !l->filelen)
    {
        loadcmod->entities = NULL;
        return;
    }
    loadcmod->entities = Hunk_AllocName(l->filelen, loadname);
    memcpy(loadcmod->entities, mod_base + l->fileofs, l->filelen);
}

/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels(lump_t *l)
{
    dmodel_t *in;
    cmodel_t *out, *next;
    int i, j, count;
    char name[10];

    in = (void *)(mod_base + l->fileofs);

    if (l->filelen % sizeof(*in))
    {
        Sys_Error("CMOD_LoadBmodel: funny lump size in %s", loadcmod->name);
    }

    count = l->filelen / sizeof(*in);
    out = loadcmod;

    loadcmod->numsubmodels = count;

    for (i = 0; i < count; i++, in++)
    {
        if (i > 0)
        {
            /* Get the next cmodel_t to be filled. */
            sprintf(name, "*%i", i);
            next = CMod_FindName(name);

            /* Duplicate the basic information. */
			*next = *out;
			strcpy(next->name, name);

            /* Switch it out. */
            out = next;
        }

        for (j = 0; j < 3; j++)
        {
            out->mins[j] = LittleFloat(in->mins[j]) - 1.0f;
            out->maxs[j] = LittleFloat(in->maxs[j]) + 1.0f;
        }

        out->hulls[HULL_POINT].firstclipnode = LittleLong(in->headnode[HULL_POINT]);

        for (j = 1; j < MAX_MAP_HULLS; j++)
        {
            out->hulls[j].firstclipnode = LittleLong(in->headnode[j]);
            out->hulls[j].lastclipnode = out->numclipnodes - 1;
        }

        out->numleafs = LittleLong(in->visleafs);
    }
}

/*
=================
CMod_SetParent
=================
*/
void CMod_SetParent(mnode_t *node, mnode_t *parent)
{
    node->parent = parent;
    if (node->contents < 0)
    {
        return;
    }
    CMod_SetParent(node->children[0], node);
    CMod_SetParent(node->children[1], node);
}

/*
=================
CMod_LoadNodes
=================
*/
void CMod_LoadNodes(lump_t *l)
{
    int i, j, count, p;
    dnode_t *in;
    mnode_t *out;

    in = (void *)(mod_base + l->fileofs);

    if (l->filelen % sizeof(*in))
    {
        Sys_Error("CMOD_LoadBmodel: funny lump size in %s", loadcmod->name);
    }

    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadcmod->nodes = out;
    loadcmod->numnodes = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        for (j = 0; j < 3; j++)
        {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->planenum);
        out->plane = loadcmod->planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);

        for (j = 0; j < 2; j++)
        {
            p = LittleShort(in->children[j]);
            if (p >= 0)
            {
                out->children[j] = loadcmod->nodes + p;
            }
            else
            {
                out->children[j] = (mnode_t *)(loadcmod->leafs + (-1 - p));
            }
        }
    }

    CMod_SetParent(loadcmod->nodes, NULL); // sets nodes and leafs
}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs(lump_t *l)
{
    dleaf_t *in;
    mleaf_t *out;
    int i, j, count, p;

    in = (void *)(mod_base + l->fileofs);

    if (l->filelen % sizeof(*in))
    {
        Sys_Error("CMOD_LoadBmodel: funny lump size in %s", loadcmod->name);
    }

    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadcmod->leafs = out;
    loadcmod->numleafs = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        for (j = 0; j < 3; j++)
        {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->contents);
        out->contents = p;

		out->firstmarksurface = LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);

		out->efrags = NULL;

		for (j = 0; j < NUM_AMBIENTS; j++)
		{
			out->ambient_sound_level[j] = in->ambient_level[j];
		}

        p = LittleLong(in->visofs);

        if (p == -1)
        {
            out->compressed_vis = NULL;
        }
        else
        {
            out->compressed_vis = loadcmod->visdata + p;
        }
    }
}

/*
=================
CMod_LoadClipnodes
=================
*/
void CMod_LoadClipnodes(lump_t *l)
{
    dclipnode_t *in, *out;
    int i, count;
    hull_t *hull;

    in = (void *)(mod_base + l->fileofs);

    if (l->filelen % sizeof(*in))
    {
        Sys_Error("CMOD_LoadBmodel: funny lump size in %s", loadcmod->name);
    }

    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadcmod->clipnodes = out;
    loadcmod->numclipnodes = count;

    for (i = 1; i < MAX_MAP_HULLS; i++)
    {
        hull = &loadcmod->hulls[i];
        hull->clipnodes = out;
        hull->firstclipnode = 0;
        hull->lastclipnode = count - 1;
        hull->planes = loadcmod->planes;

        if (mod_version == BSPQUAKE)
        {
            VectorCopy(quake_hull_sizes[i][0], hull->clip_mins);
            VectorCopy(quake_hull_sizes[i][1], hull->clip_maxs);
        }
        else
        {
            VectorCopy(hull_sizes[i][0], hull->clip_mins);
            VectorCopy(hull_sizes[i][1], hull->clip_maxs);
        }
    }

    for (i = 0; i < count; i++, out++, in++)
    {
        out->planenum = LittleLong(in->planenum);
        out->children[0] = LittleShort(in->children[0]);
        out->children[1] = LittleShort(in->children[1]);
    }
}

/*
=================
CMod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void CMod_MakeHull0()
{
    mnode_t *in, *child;
    dclipnode_t *out;
    int i, j, count;
    hull_t *hull;

    hull = &loadcmod->hulls[HULL_POINT];

    in = loadcmod->nodes;
    count = loadcmod->numnodes;
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadcmod->planes;

    for (i = 0; i < count; i++, out++, in++)
    {
        out->planenum = in->plane - loadcmod->planes;
        for (j = 0; j < 2; j++)
        {
            child = in->children[j];
            if (child->contents < 0)
            {
                out->children[j] = child->contents;
            }
            else
            {
                out->children[j] = child - loadcmod->nodes;
            }
        }
    }
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes(lump_t *l)
{
    int i, j;
    mplane_t *out;
    dplane_t *in;
    int count;
    int bits;

    in = (void *)(mod_base + l->fileofs);

    if (l->filelen % sizeof(*in))
    {
        Sys_Error("CMOD_LoadBmodel: funny lump size in %s", loadcmod->name);
    }

    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * 2 * sizeof(*out), loadname);

    loadcmod->planes = out;
    loadcmod->numplanes = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        bits = 0;
        for (j = 0; j < 3; j++)
        {
            out->normal[j] = LittleFloat(in->normal[j]);
            if (out->normal[j] < 0)
            {
                bits |= 1 << j;
            }
        }

        out->dist = LittleFloat(in->dist);
        out->type = LittleLong(in->type);
        out->signbits = bits;
    }
}

/*
=================
CMod_LoadBrushModel
=================
*/
void CMod_LoadBrushModel(cmodel_t *mod, void *buffer)
{
    int i;
    dheader_t *header;

    header = (dheader_t *)buffer;

    mod_version = LittleLong(header->version);

    if (mod_version != BSPVERSION && mod_version != BSPQUAKE)
    {
        Sys_Error(
            "CMod_LoadBrushModel: %s has wrong version number \
			(%i should be %i or %i)",
            mod->name,
            mod_version,
            BSPVERSION,
            BSPQUAKE);
    }

    // swap all the lumps
    mod_base = (byte *)header;

    for (i = 0; i < sizeof(dheader_t) / 4; i++)
    {
        ((int32_t *)header)[i] = LittleLong(((int32_t *)header)[i]);
    }

    // load into heap
    CMod_LoadEntities   (&header->lumps[LUMP_ENTITIES]);
    CMod_LoadPlanes     (&header->lumps[LUMP_PLANES]);
    CMod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
    CMod_LoadLeafs      (&header->lumps[LUMP_LEAFS]);
    CMod_LoadNodes      (&header->lumps[LUMP_NODES]);
    CMod_LoadClipnodes  (&header->lumps[LUMP_CLIPNODES]);

    CMod_MakeHull0();

    CMod_LoadSubmodels  (&header->lumps[LUMP_MODELS]);
}

//=============================================================================

/*
================
CMod_Print
================
*/
void CMod_Print()
{
    int i;
    cmodel_t *mod;

    Con_Printf("Cached cmodels:\n");
    for (i = 0, mod = cmod_known; i < cmod_numknown; i++, mod++)
    {
        Con_Printf("%u : %s\n", i, mod->name);
    }
}
