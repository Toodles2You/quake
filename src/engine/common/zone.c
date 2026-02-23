/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers
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

#include "bothdef.h"

/*
==============================================================================

ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

void Z_Free (void *ptr)
{
	free (ptr);
}

void *Z_Malloc (size_t size)
{
	return malloc (size);
}

void *Z_Realloc (void *ptr, size_t size)
{
	return realloc (ptr, size);
}

#define HUNK_SENTINAL 0x1df001ed

typedef struct
{
	size_t sentinal;
	size_t size; // including sizeof(hunk_t), 0 = not allocated
	char name[8];
} hunk_t;

static byte *hunk_base;
static size_t hunk_size;

static size_t hunk_low_used;
static size_t hunk_high_used;

static bool hunk_tempactive;
static size_t hunk_tempmark;

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check (void)
{
	hunk_t *h;

	for (h = (hunk_t *)hunk_base; (byte *)h != hunk_base + hunk_low_used;)
	{
		if (h->sentinal != HUNK_SENTINAL)
			Sys_Error ("Hunk_Check: trashed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Sys_Error ("Hunk_Check: bad size");
		h = (hunk_t *)((byte *)h + h->size);
	}
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
static void Hunk_Print (bool all)
{
	hunk_t *h, *next, *endlow, *starthigh, *endhigh;
	int count, sum;
	int totalblocks;
	char name[9];

	name[8] = 0;
	count = 0;
	sum = 0;
	totalblocks = 0;

	h = (hunk_t *)hunk_base;
	endlow = (hunk_t *)(hunk_base + hunk_low_used);
	starthigh = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	endhigh = (hunk_t *)(hunk_base + hunk_size);

	Con_Printf ("          :%8lu total hunk size\n", hunk_size);
	Con_Printf ("-------------------------\n");

	while (1)
	{
		//
		// skip to the high hunk if done with low hunk
		//
		if (h == endlow)
		{
			Con_Printf ("-------------------------\n");
			Con_Printf ("          :%8lu REMAINING\n", hunk_size - hunk_low_used - hunk_high_used);
			Con_Printf ("-------------------------\n");
			h = starthigh;
		}

		//
		// if totally done, break
		//
		if (h == endhigh)
			break;

		//
		// run consistancy checks
		//
		if (h->sentinal != HUNK_SENTINAL)
			Sys_Error ("Hunk_Check: trashed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Sys_Error ("Hunk_Check: bad size");

		next = (hunk_t *)((byte *)h + h->size);
		count++;
		totalblocks++;
		sum += h->size;

		//
		// print the single block
		//
		memcpy (name, h->name, 8);
		if (all)
			Con_Printf ("%8p :%8lu %8s\n", h, h->size, name);

		//
		// print the total
		//
		if (next == endlow || next == endhigh || strncmp (h->name, next->name, 8))
		{
			if (!all)
				Con_Printf ("          :%8lu %8s (TOTAL)\n", sum, name);
			count = 0;
			sum = 0;
		}

		h = next;
	}

	Con_Printf ("-------------------------\n");
	Con_Printf ("%8lu total blocks\n", totalblocks);
}

void *Hunk_AllocName (size_t size, char *name)
{
	hunk_t *h;

	if (size < 0)
		Sys_Error ("Hunk_Alloc: %s bad size: %lu", name, size);

	size = sizeof (hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
		Sys_Error ("Hunk_Alloc: %s failed on %lu bytes", name, size);

	h = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	memset (h, 0, size);

	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	strncpy (h->name, name, 8);

	return (void *)(h + 1);
}

void *Hunk_Alloc (size_t size)
{
	return Hunk_AllocName (size, "unknown");
}

size_t Hunk_LowMark (void)
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark (size_t mark)
{
	if (mark < 0 || mark > hunk_low_used)
		Sys_Error ("Hunk_FreeToLowMark: bad mark %lu", mark);
	memset (hunk_base + mark, 0, hunk_low_used - mark);
	hunk_low_used = mark;
}

size_t Hunk_HighMark (void)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}

	return hunk_high_used;
}

void Hunk_FreeToHighMark (size_t mark)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
		Sys_Error ("Hunk_FreeToHighMark: bad mark %lu", mark);
	memset (hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
	hunk_high_used = mark;
}

void *Hunk_HighAllocName (size_t size, char *name)
{
	hunk_t *h;

	if (size < 0)
		Sys_Error ("Hunk_HighAllocName: %s bad size: %lu", name, size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

	size = sizeof (hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		Con_Printf ("Hunk_HighAlloc: %s failed on %lu bytes\n", name, size);
		return NULL;
	}

	hunk_high_used += size;

	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);

	memset (h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	strncpy (h->name, name, 8);

	return (void *)(h + 1);
}

/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void *Hunk_TempAlloc (size_t size)
{
	void *buf;

	size = (size + 15) & ~15;

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

	hunk_tempmark = Hunk_HighMark ();

	buf = Hunk_HighAllocName (size, "temp");

	hunk_tempactive = true;

	return buf;
}

void Memory_Init (void *buf, size_t size)
{
	hunk_base = buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;
}
