
#ifndef _WORLD_H
#define _WORLD_H

typedef struct
{
	vec3_t normal;
	float dist;
} plane_t;

typedef struct
{
	bool allsolid;	 // if true, plane is not valid
	bool startsolid; // if true, the initial point was in a solid area
	bool inopen, inwater;
	float fraction; // time completed, 1.0 = didn't hit anything
	vec3_t endpos;	// final position
	plane_t plane;	// surface normal at impact
	edict_t *ent;	// entity the surface is on
} trace_t;

enum
{
	MOVE_NORMAL = 0,
	MOVE_NOMONSTERS,
	MOVE_MISSILE,
};

void SV_ClearWorld();
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict(edict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEdict(edict_t *ent, bool touch_triggers);
// Needs to be called any time an entity changes origin, mins, maxs, or solid
// sets absmin and absmax
// if touchtriggers, calls prog functions for the intersected triggers

int SV_PointContents(vec3_t p);
int SV_TruePointContents(vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all
// the non-true version remaps the water current contents to content_water

edict_t *SV_TestEntityPosition(edict_t *ent);

trace_t SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t *passedict);
// mins and maxs are reletive

// if the entire move stays in a solid volume, trace.allsolid will be set

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// nomonsters is used for line of sight or edge testing, where mosnters
// shouldn't be considered solid objects

// passedict is explicitly excluded from clipping checks (normally NULL)

#endif /* !_WORLD_H */
