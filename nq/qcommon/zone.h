/*
 memory allocation


H_??? The hunk manages the entire memory block given to quake.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object


Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.


------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/

#ifndef _ZONE_H
#define _ZONE_H

void Memory_Init(void *buf, size_t size);

void Z_Free(void *ptr);
void *Z_Malloc(size_t size); // returns 0 filled memory
void *Z_TagMalloc(size_t size, size_t tag);

void Z_CheckHeap();

void *Hunk_Alloc(size_t size); // returns 0 filled memory
void *Hunk_AllocName(size_t size, char *name);

void *Hunk_HighAllocName(size_t size, char *name);

size_t Hunk_LowMark();
void Hunk_FreeToLowMark(size_t mark);

size_t Hunk_HighMark();
void Hunk_FreeToHighMark(size_t mark);

void *Hunk_TempAlloc(size_t size);

void Hunk_Check();

typedef struct cache_user_s
{
	void *data;
} cache_user_t;

void Cache_Flush();

void *Cache_Check(cache_user_t *c);
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void Cache_Free(cache_user_t *c);

void *Cache_Alloc(cache_user_t *c, size_t size, char *name);
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.

void Cache_Report();

#endif /* !_ZONE_H */
