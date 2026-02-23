/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023-2024 Justin Keller

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

#include "clientdef.h"

int skytexturenum;

void R_InitTextures (void)
{
	int x, y, m;
	byte *dest;

	// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof (texture_t) + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2, "notexture");

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof (texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16 * 16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8 * 8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4 * 4;

	for (m = 0; m < 4; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y = 0; y < (16 >> m); y++)
			for (x = 0; x < (16 >> m); x++)
			{
				if ((y < (8 >> m)) ^ (x < (8 >> m)))
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}
}

static const byte dottexture[8][8] = {
	{1, 1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
};

static void R_InitParticleTexture (void)
{
	int x, y;
	byte data[8][8][4];

	//
	// particle texture
	//
	particletexture = texture_extension_number++;
	GL_Bind (particletexture);

	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y] * 255;
		}
	}
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
}

void R_Init (void)
{
	extern cvar_t gl_finish;

	Cvar_RegisterVariable (src_client, &r_norefresh);
	Cvar_RegisterVariable (src_client, &r_lightmap);
	Cvar_RegisterVariable (src_client, &r_fullbright);
	Cvar_RegisterVariable (src_client, &r_drawentities);
	Cvar_RegisterVariable (src_client, &r_drawviewmodel);
	Cvar_RegisterVariable (src_client, &r_wateralpha);
	Cvar_RegisterVariable (src_client, &r_dynamic);
	Cvar_RegisterVariable (src_client, &r_novis);
	Cvar_RegisterVariable (src_client, &r_speeds);
	Cvar_RegisterVariable (src_client, &r_netgraph);
	Cvar_RegisterVariable (src_client, &r_fence);
	Cvar_RegisterVariable (src_client, &r_luminescent);
	Cvar_RegisterVariable (src_client, &r_zmax);
	Cvar_RegisterVariable (src_client, &r_wireframe);

	Cvar_RegisterVariable (src_client, &gl_finish);
	Cvar_RegisterVariable (src_client, &gl_clear);
	Cvar_RegisterVariable (src_client, &gl_texsort);

	if (gl_mtexable)
		Cvar_SetValue (src_client, "gl_texsort", 0);

	Cvar_RegisterVariable (src_client, &gl_smoothmodels);
	Cvar_RegisterVariable (src_client, &gl_affinemodels);
	Cvar_RegisterVariable (src_client, &gl_polyblend);
	Cvar_RegisterVariable (src_client, &gl_playermip);
	Cvar_RegisterVariable (src_client, &gl_keeptjunctions);
	Cvar_RegisterVariable (src_client, &gl_partblend);

	R_InitParticles ();
	R_InitParticleTexture ();

	netgraphtexture = texture_extension_number;
	texture_extension_number++;
}

void R_NewMap (void)
{
	int i;

	for (i = 0; i < 256; i++)
		d_lightstylevalue[i] = 264; // normal light value

	memset (&r_worldentity, 0, sizeof (r_worldentity));
	r_worldentity.model = cl.worldmodel;

	// clear out efrags in case the level hasn't been reloaded
	// FIXME: is this one short?
	for (i = 0; i < cl.cmodel_precache[1]->numleafs; i++)
		cl.cmodel_precache[1]->leafs[i].efrags = NULL;

	r_viewleaf = NULL;
	R_ClearParticles ();

	GL_BuildLightmaps ();

	// identify sky texture
	skytexturenum = -1;
	for (i = 0; i < BMODEL (cl.worldmodel)->numtextures; i++)
	{
		if (!BMODEL (cl.worldmodel)->textures[i])
			continue;
		if (!strncmp (BMODEL (cl.worldmodel)->textures[i]->name, "sky", 3))
			skytexturenum = i;
		BMODEL (cl.worldmodel)->textures[i]->texturechain = NULL;
	}
}
