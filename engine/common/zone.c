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

static void Cache_FreeLow (size_t new_low_hunk);
static void Cache_FreeHigh (size_t new_high_hunk);

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
		Sys_Error ("Hunk_Alloc: bad size: %lu", size);

	size = sizeof (hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
		Sys_Error ("Hunk_Alloc: failed on %lu bytes", size);

	h = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Cache_FreeLow (hunk_low_used);

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
		Sys_Error ("Hunk_HighAllocName: bad size: %lu", size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

	size = sizeof (hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		Con_Printf ("Hunk_HighAlloc: failed on %lu bytes\n", size);
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh (hunk_high_used);

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

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

typedef struct cache_system_s
{
	size_t size; // including this header
	cache_user_t *user;
	char name[16];
	struct cache_system_s *prev, *next;
	struct cache_system_s *lru_prev, *lru_next; // for LRU flushing
} cache_system_t;

static cache_system_t *Cache_TryAlloc (size_t size, bool nobottom);

static cache_system_t cache_head;

static void Cache_Move (cache_system_t *c)
{
	cache_system_t *new;

	// we are clearing up space at the bottom, so only allocate it late
	new = Cache_TryAlloc (c->size, true);
	if (new)
	{
		memcpy (new + 1, c + 1, c->size - sizeof (cache_system_t));
		new->user = c->user;
		memcpy (new->name, c->name, sizeof (new->name));
		Cache_Free (c->user);
		new->user->data = (void *)(new + 1);
	}
	else
	{
		Cache_Free (c->user); // tough luck...
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
static void Cache_FreeLow (size_t new_low_hunk)
{
	cache_system_t *c;

	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return; // nothing in cache at all
		if ((byte *)c >= hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move (c); // reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
static void Cache_FreeHigh (size_t new_high_hunk)
{
	cache_system_t *c, *prev;

	prev = NULL;
	while (1)
	{
		c = cache_head.prev;
		if (c == &cache_head)
			return; // nothing in cache at all
		if ((byte *)c + c->size <= hunk_base + hunk_size - new_high_hunk)
			return; // there is space to grow the hunk
		if (c == prev)
			Cache_Free (c->user); // didn't move out of the way
		else
		{
			Cache_Move (c); // try to move it
			prev = c;
		}
	}
}

static void Cache_UnlinkLRU (cache_system_t *cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		Sys_Error ("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

static void Cache_MakeLRU (cache_system_t *cs)
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error ("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
static cache_system_t *Cache_TryAlloc (size_t size, bool nobottom)
{
	cache_system_t *cs, *new;

	// is the cache completely empty?

	if (!nobottom && cache_head.prev == &cache_head)
	{
		if (hunk_size - hunk_high_used - hunk_low_used < size)
			Sys_Error ("Cache_TryAlloc: %lu is greater then free hunk", size);

		new = (cache_system_t *)(hunk_base + hunk_low_used);
		memset (new, 0, sizeof (*new));
		new->size = size;

		cache_head.prev = cache_head.next = new;
		new->prev = new->next = &cache_head;

		Cache_MakeLRU (new);
		return new;
	}

	// search from the bottom up for space

	new = (cache_system_t *)(hunk_base + hunk_low_used);
	cs = cache_head.next;

	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ((byte *)cs - (byte *)new >= size)
			{ // found space
				memset (new, 0, sizeof (*new));
				new->size = size;

				new->next = cs;
				new->prev = cs->prev;
				cs->prev->next = new;
				cs->prev = new;

				Cache_MakeLRU (new);

				return new;
			}
		}

		// continue looking
		new = (cache_system_t *)((byte *)cs + cs->size);
		cs = cs->next;

	} while (cs != &cache_head);

	// try to allocate one at the very end
	if (hunk_base + hunk_size - hunk_high_used - (byte *)new >= size)
	{
		memset (new, 0, sizeof (*new));
		new->size = size;

		new->next = &cache_head;
		new->prev = cache_head.prev;
		cache_head.prev->next = new;
		cache_head.prev = new;

		Cache_MakeLRU (new);

		return new;
	}

	return NULL; // couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
void Cache_Flush (void)
{
	while (cache_head.next != &cache_head)
		Cache_Free (cache_head.next->user); // reclaim the space
}

static void Cache_Print (void)
{
	cache_system_t *cd;

	for (cd = cache_head.next; cd != &cache_head; cd = cd->next)
		Con_Printf ("%8lu : %s\n", cd->size, cd->name);
}

void Cache_Report (void)
{
	Con_DPrintf ("%4.1f megabyte data cache\n", (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024 * 1024));
}

static void Cache_Init (void)
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;

	Cmd_AddCommand (src_host, "flush", Cache_Flush);
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free (cache_user_t *c)
{
	cache_system_t *cs;

	if (!c->data)
		Sys_Error ("Cache_Free: not allocated");

	cs = ((cache_system_t *)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU (cs);
}

void *Cache_Check (cache_user_t *c)
{
	cache_system_t *cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *)c->data) - 1;

	// move to head of LRU
	Cache_UnlinkLRU (cs);
	Cache_MakeLRU (cs);

	return c->data;
}

void *Cache_Alloc (cache_user_t *c, size_t size, char *name)
{
	cache_system_t *cs;

	if (c->data)
		Sys_Error ("Cache_Alloc: allready allocated");

	if (size <= 0)
		Sys_Error ("Cache_Alloc: size %lu", size);

	size = (size + sizeof (cache_system_t) + 15) & ~15;

	// find memory for it
	while (1)
	{
		cs = Cache_TryAlloc (size, false);
		if (cs)
		{
			strncpy (cs->name, name, sizeof (cs->name) - 1);
			c->data = (void *)(cs + 1);
			cs->user = c;
			break;
		}

		// free the least recently used cahedat
		if (cache_head.lru_prev == &cache_head)
			Sys_Error ("Cache_Alloc: out of memory");
		// not enough memory at all
		Cache_Free (cache_head.lru_prev->user);
	}

	return Cache_Check (c);
}

void Memory_Init (void *buf, size_t size)
{
	size_t p;

	hunk_base = buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;

	Cache_Init ();
}
