/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// wad.c

#include "quakedef.h"

int			wad_numlumps;
lumpinfo_t	*wad_lumps;
byte		*wad_base;

void SwapPic (qpic_t *pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName (char *in, char *out)
{
	int		i;
	int		c;
	
	for (i=0 ; i<16 ; i++ )
	{
		c = in[i];
		if (!c)
			break;
			
		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		out[i] = c;
	}
	
	for ( ; i< 16 ; i++ )
		out[i] = 0;
}



/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile (char *filename)
{
	lumpinfo_t		*lump_p;
	wadinfo_t		*header;
	unsigned		i;
	int				infotableofs;
	
	wad_base = COM_LoadHunkFile (filename);
	if (!wad_base)
		Sys_Error ("W_LoadWadFile: couldn't load %s", filename);

	header = (wadinfo_t *)wad_base;
	
	if (header->identification[0] != 'W'
	|| header->identification[1] != 'A'
	|| header->identification[2] != 'D'
	|| header->identification[3] != '2')
		Sys_Error ("Wad file %s doesn't have WAD2 id\n",filename);
		
	wad_numlumps = LittleLong(header->numlumps);
	infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_t *)(wad_base + infotableofs);
	
	for (i=0, lump_p = wad_lumps ; i<wad_numlumps ; i++,lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName (lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
			SwapPic ( (qpic_t *)(wad_base + lump_p->filepos));
	}
}


/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t	*W_GetLumpinfo (char *name)
{
	int		i;
	lumpinfo_t	*lump_p;
	char	clean[16];
	
	W_CleanupName (name, clean);
	
	for (lump_p=wad_lumps, i=0 ; i<wad_numlumps ; i++,lump_p++)
	{
		if (!strcmp(clean, lump_p->name))
			return lump_p;
	}
	
	Sys_Error ("W_GetLumpinfo: %s not found", name);
	return NULL;
}

void *W_GetLumpName (char *name)
{
	lumpinfo_t	*lump;
	
	lump = W_GetLumpinfo (name);
	
	return (void *)(wad_base + lump->filepos);
}

void *W_GetLumpNum (int num)
{
	lumpinfo_t	*lump;
	
	if (num < 0 || num > wad_numlumps)
		Sys_Error ("W_GetLumpNum: bad number: %i", num);
		
	lump = wad_lumps + num;
	
	return (void *)(wad_base + lump->filepos);
}

typedef struct wad_s wad_t;

typedef struct wad_s {
	char		name[16];
	wad_t		*next;
	int			handle;
	int			numlumps;
	lumpinfo_t	lumps[];
} wad_t;

static wad_t* wad_head = NULL;

/*
====================
W_LoadMapWadFile
====================
*/
void W_LoadMapWadFile(char *filename)
{
	lumpinfo_t *lump_p;
	unsigned i;
	int infotableofs;

	int handle;

	if (COM_OpenFile(filename, &handle) == -1)
	{
		Con_DPrintf("W_LoadWadFile: couldn't load %s\n", filename);
		return;
	}

	wadinfo_t header;
	
	Sys_FileSeek(handle, 0);
	if (Sys_FileRead(handle, &header, sizeof(header)) <= 0)
	{
		Con_DPrintf("W_LoadWadFile: couldn't read %s\n", filename);
		return;
	}

	if (header.identification[0] != 'W'
		|| header.identification[1] != 'A'
		|| header.identification[2] != 'D'
		|| header.identification[3] != '2')
	{
		Con_DPrintf("Wad file %s doesn't have WAD2 id\n", filename);
		return;
	}

	Con_DPrintf("%s\n", filename);

	int numlumps = LittleLong(header.numlumps);

	wad_t *wad = Hunk_Alloc(sizeof(wad_t) + sizeof(lumpinfo_t) * numlumps);
	Q_strncpy (wad->name, filename, 16);
	wad->numlumps = numlumps;
	wad->handle = handle;
	wad->next = wad_head;
	wad_head = wad;

	infotableofs = LittleLong(header.infotableofs);
	
	Sys_FileSeek(wad->handle, infotableofs);
	if (Sys_FileRead(wad->handle, &wad->lumps[0], sizeof(lumpinfo_t) * wad->numlumps) <= 0)
	{
		Con_DPrintf("W_LoadWadFile: couldn't read %s\n", filename);
		return;
	}

	for (i = 0, lump_p = wad->lumps; i < wad->numlumps; i++, lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName(lump_p->name, lump_p->name);
	}
}

static qboolean W_GetMapLumpInfo(char *name, wad_t** wad, lumpinfo_t **lump)
{
	if (!wad_head)
		return false;
	
	int i;
	lumpinfo_t *lump_p;
	char clean[16];

	W_CleanupName(name, clean);

	wad_t* wad_p;

	for (wad_p = wad_head; wad_p != NULL; wad_p = wad_p->next)
	{
		for (lump_p = wad_p->lumps, i = 0; i < wad_p->numlumps; i++, lump_p++)
		{
			if (!Q_strcmp(clean, lump_p->name))
			{
				*wad = wad_p;
				*lump = lump_p;
				/* Con_DSafePrintf("W_GetLumpinfo: %s found in %s\n", clean, wad_p->name); */
				return true;
			}
		}
	}

	Con_DPrintf("W_GetLumpinfo: %s not found\n", clean);
	return false;
}

static wad_t* mip_wad = NULL;
static lumpinfo_t* mip_lump = NULL;

qboolean W_LoadMapMiptexInfo(char *name, void* dst)
{
	if (!W_GetMapLumpInfo(name, &mip_wad, &mip_lump))
	{
		return false;
	}
	
	Sys_FileSeek(mip_wad->handle, mip_lump->filepos);
	return Sys_FileRead(mip_wad->handle, dst, sizeof(miptex_t)) > 0;
}

qboolean W_LoadMapMiptexData(void* dst)
{
	if (!mip_wad || !mip_lump)
	{
		return false;
	}

	Sys_FileSeek(mip_wad->handle, mip_lump->filepos + sizeof(miptex_t));
	qboolean result = Sys_FileRead(mip_wad->handle, dst, mip_lump->size - sizeof(miptex_t)) > 0;
	
	mip_wad = NULL;
	mip_lump = NULL;
	
	return result;
}

/*
====================
W_FreeMapWadFiles
====================
*/
void W_FreeMapWadFiles()
{
	wad_t* wad = wad_head;
	wad_t* next;

	if (!wad_head)
		return;

	do
	{
		COM_CloseFile(wad->handle);
		next = wad->next;
	}
	while (wad = next);

	wad_head = NULL;
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic (qpic_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);	
}
