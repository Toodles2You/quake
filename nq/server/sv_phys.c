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

#include "serverdef.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects 

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

cvar_t	sv_friction = {"sv_friction","4",false,true};
cvar_t	sv_stopspeed = {"sv_stopspeed","100"};
cvar_t	sv_gravity = {"sv_gravity","800",false,true};
cvar_t	sv_maxvelocity = {"sv_maxvelocity","2000"};
cvar_t	sv_nostep = {"sv_nostep","0"};

#ifdef QUAKE2
static	vec3_t	vec_origin = {0.0, 0.0, 0.0};
#endif

#define	MOVE_EPSILON	0.01

void SV_Physics_Toss (edict_t *ent);

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts ()
{
	int			e;
	edict_t		*check;

// see if any solid entities are inside the final position
	check = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
			continue;
		if (ed_float(check, movetype) == MOVETYPE_PUSH
		|| ed_float(check, movetype) == MOVETYPE_NONE
		|| ed_float(check, movetype) == MOVETYPE_FOLLOW
		|| ed_float(check, movetype) == MOVETYPE_NOCLIP)
			continue;

		if (SV_TestEntityPosition (check))
			Con_Printf ("entity in invalid position\n");
	}
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity (edict_t *ent)
{
	int		i;

//
// bound velocity
//
	for (i=0 ; i<3 ; i++)
	{
		if (IS_NAN(ed_vector(ent, velocity)[i]))
		{
			Con_Printf ("Got a NaN velocity on %s\n", ed_get_string(ent, classname));
			ed_vector(ent, velocity)[i] = 0;
		}
		if (IS_NAN(ed_vector(ent, origin)[i]))
		{
			Con_Printf ("Got a NaN origin on %s\n", ed_get_string(ent, classname));
			ed_vector(ent, origin)[i] = 0;
		}
		if (ed_vector(ent, velocity)[i] > sv_maxvelocity.value)
			ed_vector(ent, velocity)[i] = sv_maxvelocity.value;
		else if (ed_vector(ent, velocity)[i] < -sv_maxvelocity.value)
			ed_vector(ent, velocity)[i] = -sv_maxvelocity.value;
	}
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
bool SV_RunThink (edict_t *ent)
{
	float	thinktime;

	thinktime = ed_float(ent, nextthink);
	if (thinktime <= 0 || thinktime > sv.time + host_frametime)
		return true;
		
	if (thinktime < sv.time)
		thinktime = sv.time;	// don't let things stay in the past.
								// it is possible to start that way
								// by a trigger with a local time.
	ed_float(ent, nextthink) = 0;
	sv_pr_float(time) = thinktime;
	sv_pr_int(self) = EDICT_TO_PROG(ent);
	sv_pr_int(other) = EDICT_TO_PROG(sv.edicts);
	PR_ExecuteProgram (&sv.pr, ed_int(ent, think));
	return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact (edict_t *e1, edict_t *e2)
{
	int		old_self, old_other;
	
	old_self = sv_pr_int(self);
	old_other = sv_pr_int(other);
	
	sv_pr_float(time) = sv.time;
	if (ed_int(e1, touch) && ed_float(e1, solid) != SOLID_NOT)
	{
		sv_pr_int(self) = EDICT_TO_PROG(e1);
		sv_pr_int(other) = EDICT_TO_PROG(e2);
		PR_ExecuteProgram (&sv.pr, ed_int(e1, touch));
	}
	
	if (ed_int(e2, touch) && ed_float(e2, solid) != SOLID_NOT)
	{
		sv_pr_int(self) = EDICT_TO_PROG(e2);
		sv_pr_int(other) = EDICT_TO_PROG(e1);
		PR_ExecuteProgram (&sv.pr, ed_int(e2, touch));
	}

	sv_pr_int(self) = old_self;
	sv_pr_int(other) = old_other;
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

int ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i, blocked;
	
	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step
	
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
	
	return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
#define	MAX_CLIP_PLANES	5
int SV_FlyMove (edict_t *ent, float time, trace_t *steptrace)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;
	
	numbumps = 4;
	
	blocked = 0;
	VectorCopy (ed_vector(ent, velocity), original_velocity);
	VectorCopy (ed_vector(ent, velocity), primal_velocity);
	numplanes = 0;
	
	time_left = time;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		if (!ed_vector(ent, velocity)[0] && !ed_vector(ent, velocity)[1] && !ed_vector(ent, velocity)[2])
			break;

		for (i=0 ; i<3 ; i++)
			end[i] = ed_vector(ent, origin)[i] + time_left * ed_vector(ent, velocity)[i];

		trace = SV_Move (ed_vector(ent, origin), ed_vector(ent, mins), ed_vector(ent, maxs), end, false, ent);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy (vec3_origin, ed_vector(ent, velocity));
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, ed_vector(ent, origin));
			VectorCopy (ed_vector(ent, velocity), original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		if (!trace.ent)
			Sys_Error ("SV_FlyMove: !trace.ent");

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (ed_float(trace.ent, solid) == SOLID_BSP)
			{
				ed_float(ent, flags) = (int)ed_float(ent, flags) | FL_ONGROUND;
				ed_set_edict(ent, groundentity, trace.ent);
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

//
// run the impact function
//
		SV_Impact (ent, trace.ent);
		if (ent->free)
			break;		// removed by the impact function

		
		time_left -= time_left * trace.fraction;
		
	// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy (vec3_origin, ed_vector(ent, velocity));
			return 3;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i=0 ; i<numplanes ; i++)
		{
			ClipVelocity (original_velocity, planes[i], new_velocity, 1);
			for (j=0 ; j<numplanes ; j++)
				if (j != i)
				{
					if (DotProduct (new_velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}
		
		if (i != numplanes)
		{	// go along this plane
			VectorCopy (new_velocity, ed_vector(ent, velocity));
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy (vec3_origin, ed_vector(ent, velocity));
				return 7;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, ed_vector(ent, velocity));
			VectorScale (dir, d, ed_vector(ent, velocity));
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct (ed_vector(ent, velocity), primal_velocity) <= 0)
		{
			VectorCopy (vec3_origin, ed_vector(ent, velocity));
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity (edict_t *ent)
{
	float	ent_gravity;

#ifdef QUAKE2
	if (ed_float(ent, gravity))
		ent_gravity = ed_float(ent, gravity);
	else
		ent_gravity = 1.0;
#else
	eval_t	*val;

	val = GetEdictFieldValue(ent, "gravity");
	if (val && val->_float)
		ent_gravity = val->_float;
	else
		ent_gravity = 1.0;
#endif
	ed_vector(ent, velocity)[2] -= ent_gravity * sv_gravity.value * host_frametime;
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity (edict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	end;
		
	VectorAdd (ed_vector(ent, origin), push, end);

	if (ed_float(ent, movetype) == MOVETYPE_FLYMISSILE)
		trace = SV_Move (ed_vector(ent, origin), ed_vector(ent, mins), ed_vector(ent, maxs), end, MOVE_MISSILE, ent);
	else if (ed_float(ent, solid) == SOLID_TRIGGER || ed_float(ent, solid) == SOLID_NOT)
	// only clip against bmodels
		trace = SV_Move (ed_vector(ent, origin), ed_vector(ent, mins), ed_vector(ent, maxs), end, MOVE_NOMONSTERS, ent);
	else
		trace = SV_Move (ed_vector(ent, origin), ed_vector(ent, mins), ed_vector(ent, maxs), end, MOVE_NORMAL, ent);	
	
	VectorCopy (trace.endpos, ed_vector(ent, origin));
	SV_LinkEdict (ent, true);

	if (trace.ent)
		SV_Impact (ent, trace.ent);		

	return trace;
}					


/*
============
SV_PushMove

============
*/
void SV_PushMove (edict_t *pusher, float movetime)
{
	int			i, e;
	edict_t		*check, *block;
	vec3_t		mins, maxs, move;
	vec3_t		entorig, pushorig;
	int			num_moved;
	edict_t		*moved_edict[MAX_EDICTS];
	vec3_t		moved_from[MAX_EDICTS];

	if (!ed_vector(pusher, velocity)[0] && !ed_vector(pusher, velocity)[1] && !ed_vector(pusher, velocity)[2])
	{
		ed_float(pusher, ltime) += movetime;
		return;
	}

	for (i=0 ; i<3 ; i++)
	{
		move[i] = ed_vector(pusher, velocity)[i] * movetime;
		mins[i] = ed_vector(pusher, absmin)[i] + move[i];
		maxs[i] = ed_vector(pusher, absmax)[i] + move[i];
	}

	VectorCopy (ed_vector(pusher, origin), pushorig);
	
// move the pusher to it's final position

	VectorAdd (ed_vector(pusher, origin), move, ed_vector(pusher, origin));
	ed_float(pusher, ltime) += movetime;
	SV_LinkEdict (pusher, false);


// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
			continue;
		if (ed_float(check, movetype) == MOVETYPE_PUSH
		|| ed_float(check, movetype) == MOVETYPE_NONE
		|| ed_float(check, movetype) == MOVETYPE_FOLLOW
		|| ed_float(check, movetype) == MOVETYPE_NOCLIP)
			continue;

	// if the entity is standing on the pusher, it will definately be moved
		if ( ! ( ((int)ed_float(check, flags) & FL_ONGROUND)
		&& ed_get_edict(check, groundentity) == pusher) )
		{
			if (ed_vector(check, absmin)[0] >= maxs[0]
			 || ed_vector(check, absmin)[1] >= maxs[1]
			 || ed_vector(check, absmin)[2] >= maxs[2]
			 || ed_vector(check, absmax)[0] <= mins[0]
			 || ed_vector(check, absmax)[1] <= mins[1]
			 || ed_vector(check, absmax)[2] <= mins[2])
				continue;

		// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition (check))
				continue;
		}

	// remove the onground flag for non-players
		if (ed_float(check, movetype) != MOVETYPE_WALK)
			ed_float(check, flags) = (int)ed_float(check, flags) & ~FL_ONGROUND;
		
		VectorCopy (ed_vector(check, origin), entorig);
		VectorCopy (ed_vector(check, origin), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity 
		ed_float(pusher, solid) = SOLID_NOT;
		SV_PushEntity (check, move);
		ed_float(pusher, solid) = SOLID_BSP;

	// if it is still inside the pusher, block
		block = SV_TestEntityPosition (check);
		if (block)
		{	// fail the move
			if (ed_vector(check, mins)[0] == ed_vector(check, maxs)[0])
				continue;
			if (ed_float(check, solid) == SOLID_NOT || ed_float(check, solid) == SOLID_TRIGGER)
			{	// corpse
				ed_vector(check, mins)[0] = ed_vector(check, mins)[1] = 0;
				VectorCopy (ed_vector(check, mins), ed_vector(check, maxs));
				continue;
			}
			
			VectorCopy (entorig, ed_vector(check, origin));
			SV_LinkEdict (check, true);

			VectorCopy (pushorig, ed_vector(pusher, origin));
			SV_LinkEdict (pusher, false);
			ed_float(pusher, ltime) -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (ed_int(pusher, blocked))
			{
				sv_pr_int(self) = EDICT_TO_PROG(pusher);
				sv_pr_int(other) = EDICT_TO_PROG(check);
				PR_ExecuteProgram (&sv.pr, ed_int(pusher, blocked));
			}
			
		// move back any entities we already moved
			for (i=0 ; i<num_moved ; i++)
			{
				VectorCopy (moved_from[i], ed_vector(moved_edict[i], origin));
				SV_LinkEdict (moved_edict[i], false);
			}
			return;
		}	
	}

	
}

/*
============
SV_PushRotate

============
*/
void SV_PushRotate (edict_t *pusher, float movetime)
{
	int			i, e;
	edict_t		*check, *block;
	vec3_t		move, a, amove;
	vec3_t		entorig, pushorig;
	int			num_moved;
	edict_t		*moved_edict[MAX_EDICTS];
	vec3_t		moved_from[MAX_EDICTS];
	vec3_t		org, org2;
	vec3_t		forward, right, up;

	if (!ed_vector(pusher, avelocity)[0] && !ed_vector(pusher, avelocity)[1] && !ed_vector(pusher, avelocity)[2])
	{
		ed_float(pusher, ltime) += movetime;
		return;
	}

	for (i=0 ; i<3 ; i++)
		amove[i] = ed_vector(pusher, avelocity)[i] * movetime;

	VectorSubtract (vec3_origin, amove, a);
	AngleVectors (a, forward, right, up);

	VectorCopy (ed_vector(pusher, angles), pushorig);
	
// move the pusher to it's final position

	VectorAdd (ed_vector(pusher, angles), amove, ed_vector(pusher, angles));
	ed_float(pusher, ltime) += movetime;
	SV_LinkEdict (pusher, false);


// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
			continue;
		if (ed_float(check, movetype) == MOVETYPE_PUSH
		|| ed_float(check, movetype) == MOVETYPE_NONE
		|| ed_float(check, movetype) == MOVETYPE_FOLLOW
		|| ed_float(check, movetype) == MOVETYPE_NOCLIP)
			continue;

	// if the entity is standing on the pusher, it will definately be moved
		if ( ! ( ((int)ed_float(check, flags) & FL_ONGROUND)
		&& ed_get_edict(check, groundentity) == pusher) )
		{
			if ( ed_vector(check, absmin)[0] >= ed_vector(pusher, absmax)[0]
			|| ed_vector(check, absmin)[1] >= ed_vector(pusher, absmax)[1]
			|| ed_vector(check, absmin)[2] >= ed_vector(pusher, absmax)[2]
			|| ed_vector(check, absmax)[0] <= ed_vector(pusher, absmin)[0]
			|| ed_vector(check, absmax)[1] <= ed_vector(pusher, absmin)[1]
			|| ed_vector(check, absmax)[2] <= ed_vector(pusher, absmin)[2] )
				continue;

		// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition (check))
				continue;
		}

	// remove the onground flag for non-players
		if (ed_float(check, movetype) != MOVETYPE_WALK)
			ed_float(check, flags) = (int)ed_float(check, flags) & ~FL_ONGROUND;
		
		VectorCopy (ed_vector(check, origin), entorig);
		VectorCopy (ed_vector(check, origin), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// calculate destination position
		VectorSubtract (ed_vector(check, origin), ed_vector(pusher, origin), org);
		org2[0] = DotProduct (org, forward);
		org2[1] = -DotProduct (org, right);
		org2[2] = DotProduct (org, up);
		VectorSubtract (org2, org, move);

		// try moving the contacted entity 
		ed_float(pusher, solid) = SOLID_NOT;
		SV_PushEntity (check, move);
		ed_float(pusher, solid) = SOLID_BSP;

	// if it is still inside the pusher, block
		block = SV_TestEntityPosition (check);
		if (block)
		{	// fail the move
			if (ed_vector(check, mins)[0] == ed_vector(check, maxs)[0])
				continue;
			if (ed_float(check, solid) == SOLID_NOT || ed_float(check, solid) == SOLID_TRIGGER)
			{	// corpse
				ed_vector(check, mins)[0] = ed_vector(check, mins)[1] = 0;
				VectorCopy (ed_vector(check, mins), ed_vector(check, maxs));
				continue;
			}
			
			VectorCopy (entorig, ed_vector(check, origin));
			SV_LinkEdict (check, true);

			VectorCopy (pushorig, ed_vector(pusher, angles));
			SV_LinkEdict (pusher, false);
			ed_float(pusher, ltime) -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (ed_int(pusher, blocked))
			{
				sv_pr_int(self) = EDICT_TO_PROG(pusher);
				sv_pr_int(other) = EDICT_TO_PROG(check);
				PR_ExecuteProgram (&sv.pr, ed_int(pusher, blocked));
			}
			
		// move back any entities we already moved
			for (i=0 ; i<num_moved ; i++)
			{
				VectorCopy (moved_from[i], ed_vector(moved_edict[i], origin));
				VectorSubtract (ed_vector(moved_edict[i], angles), amove, ed_vector(moved_edict[i], angles));
				SV_LinkEdict (moved_edict[i], false);
			}
			return;
		}
		else
		{
			VectorAdd (ed_vector(check, angles), amove, ed_vector(check, angles));
		}
	}

	
}

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher (edict_t *ent)
{
	float	thinktime;
	float	oldltime;
	float	movetime;

	oldltime = ed_float(ent, ltime);
	
	thinktime = ed_float(ent, nextthink);
	if (thinktime < ed_float(ent, ltime) + host_frametime)
	{
		movetime = thinktime - ed_float(ent, ltime);
		if (movetime < 0)
			movetime = 0;
	}
	else
		movetime = host_frametime;

	if (movetime)
	{
		if (ed_vector(ent, avelocity)[0] || ed_vector(ent, avelocity)[1] || ed_vector(ent, avelocity)[2])
			SV_PushRotate (ent, movetime);
		else
			SV_PushMove (ent, movetime);	// advances ed_float(ent, ltime) if not blocked
	}
		
	if (thinktime > oldltime && thinktime <= ed_float(ent, ltime))
	{
		ed_float(ent, nextthink) = 0;
		sv_pr_float(time) = sv.time;
		sv_pr_int(self) = EDICT_TO_PROG(ent);
		sv_pr_int(other) = EDICT_TO_PROG(sv.edicts);
		PR_ExecuteProgram (&sv.pr, ed_int(ent, think));
		if (ent->free)
			return;
	}

}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SV_CheckStuck (edict_t *ent)
{
	int		i, j;
	int		z;
	vec3_t	org;

	if (!SV_TestEntityPosition(ent))
	{
		VectorCopy (ed_vector(ent, origin), ed_vector(ent, oldorigin));
		return;
	}

	VectorCopy (ed_vector(ent, origin), org);
	VectorCopy (ed_vector(ent, oldorigin), ed_vector(ent, origin));
	if (!SV_TestEntityPosition(ent))
	{
		Con_DPrintf ("Unstuck.\n");
		SV_LinkEdict (ent, true);
		return;
	}
	
	for (z=0 ; z< 18 ; z++)
		for (i=-1 ; i <= 1 ; i++)
			for (j=-1 ; j <= 1 ; j++)
			{
				ed_vector(ent, origin)[0] = org[0] + i;
				ed_vector(ent, origin)[1] = org[1] + j;
				ed_vector(ent, origin)[2] = org[2] + z;
				if (!SV_TestEntityPosition(ent))
				{
					Con_DPrintf ("Unstuck.\n");
					SV_LinkEdict (ent, true);
					return;
				}
			}
			
	VectorCopy (org, ed_vector(ent, origin));
	Con_DPrintf ("player is stuck.\n");
}


/*
=============
SV_CheckWater
=============
*/
bool SV_CheckWater (edict_t *ent)
{
	vec3_t	point;
	int		cont;
#ifdef QUAKE2
	int		truecont;
#endif

	point[0] = ed_vector(ent, origin)[0];
	point[1] = ed_vector(ent, origin)[1];
	point[2] = ed_vector(ent, origin)[2] + ed_vector(ent, mins)[2] + 1;	
	
	ed_float(ent, waterlevel) = 0;
	ed_float(ent, watertype) = CONTENTS_EMPTY;
	cont = SV_PointContents (point);
	if (cont <= CONTENTS_WATER)
	{
#ifdef QUAKE2
		truecont = SV_TruePointContents (point);
#endif
		ed_float(ent, watertype) = cont;
		ed_float(ent, waterlevel) = 1;
		point[2] = ed_vector(ent, origin)[2] + (ed_vector(ent, mins)[2] + ed_vector(ent, maxs)[2])*0.5;
		cont = SV_PointContents (point);
		if (cont <= CONTENTS_WATER)
		{
			ed_float(ent, waterlevel) = 2;
			point[2] = ed_vector(ent, origin)[2] + ed_vector(ent, view_ofs)[2];
			cont = SV_PointContents (point);
			if (cont <= CONTENTS_WATER)
				ed_float(ent, waterlevel) = 3;
		}
#ifdef QUAKE2
		if (truecont <= CONTENTS_CURRENT_0 && truecont >= CONTENTS_CURRENT_DOWN)
		{
			static vec3_t current_table[] =
			{
				{1, 0, 0},
				{0, 1, 0},
				{-1, 0, 0},
				{0, -1, 0},
				{0, 0, 1},
				{0, 0, -1}
			};

			VectorMA (ed_vector(ent, basevelocity), 150.0*ed_float(ent, waterlevel)/3.0, current_table[CONTENTS_CURRENT_0 - truecont], ed_vector(ent, basevelocity));
		}
#endif
	}
	
	return ed_float(ent, waterlevel) > 1;
}

/*
============
SV_WallFriction

============
*/
void SV_WallFriction (edict_t *ent, trace_t *trace)
{
	vec3_t		forward, right, up;
	float		d, i;
	vec3_t		into, side;
	
	AngleVectors (ed_vector(ent, v_angle), forward, right, up);
	d = DotProduct (trace->plane.normal, forward);
	
	d += 0.5;
	if (d >= 0)
		return;
		
// cut the tangential velocity
	i = DotProduct (trace->plane.normal, ed_vector(ent, velocity));
	VectorScale (trace->plane.normal, i, into);
	VectorSubtract (ed_vector(ent, velocity), into, side);
	
	ed_vector(ent, velocity)[0] = side[0] * (1 + d);
	ed_vector(ent, velocity)[1] = side[1] * (1 + d);
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SV_TryUnstick (edict_t *ent, vec3_t oldvel)
{
	int		i;
	vec3_t	oldorg;
	vec3_t	dir;
	int		clip;
	trace_t	steptrace;
	
	VectorCopy (ed_vector(ent, origin), oldorg);
	VectorCopy (vec3_origin, dir);

	for (i=0 ; i<8 ; i++)
	{
// try pushing a little in an axial direction
		switch (i)
		{
			case 0:	dir[0] = 2; dir[1] = 0; break;
			case 1:	dir[0] = 0; dir[1] = 2; break;
			case 2:	dir[0] = -2; dir[1] = 0; break;
			case 3:	dir[0] = 0; dir[1] = -2; break;
			case 4:	dir[0] = 2; dir[1] = 2; break;
			case 5:	dir[0] = -2; dir[1] = 2; break;
			case 6:	dir[0] = 2; dir[1] = -2; break;
			case 7:	dir[0] = -2; dir[1] = -2; break;
		}
		
		SV_PushEntity (ent, dir);

// retry the original move
		ed_vector(ent, velocity)[0] = oldvel[0];
		ed_vector(ent, velocity)[1] = oldvel[1];
		ed_vector(ent, velocity)[2] = 0;
		clip = SV_FlyMove (ent, 0.1, &steptrace);

		if ( fabs(oldorg[1] - ed_vector(ent, origin)[1]) > 4
		|| fabs(oldorg[0] - ed_vector(ent, origin)[0]) > 4 )
		{
//Con_DPrintf ("unstuck!\n");
			return clip;
		}
			
// go back to the original pos and try again
		VectorCopy (oldorg, ed_vector(ent, origin));
	}
	
	VectorCopy (vec3_origin, ed_vector(ent, velocity));
	return 7;		// still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
#define	STEPSIZE	18
void SV_WalkMove (edict_t *ent)
{
	vec3_t		upmove, downmove;
	vec3_t		oldorg, oldvel;
	vec3_t		nosteporg, nostepvel;
	int			clip;
	int			oldonground;
	trace_t		steptrace, downtrace;
	
//
// do a regular slide move unless it looks like you ran into a step
//
	oldonground = (int)ed_float(ent, flags) & FL_ONGROUND;
	ed_float(ent, flags) = (int)ed_float(ent, flags) & ~FL_ONGROUND;
	
	VectorCopy (ed_vector(ent, origin), oldorg);
	VectorCopy (ed_vector(ent, velocity), oldvel);
	
	clip = SV_FlyMove (ent, host_frametime, &steptrace);

	if ( !(clip & 2) )
		return;		// move didn't block on a step

	if (!oldonground && ed_float(ent, waterlevel) == 0)
		return;		// don't stair up while jumping
	
	if (ed_float(ent, movetype) != MOVETYPE_WALK)
		return;		// gibbed by a trigger
	
	if (sv_nostep.value)
		return;
	
	if ((int)ed_float(ent, flags) & FL_WATERJUMP)
		return;

	VectorCopy (ed_vector(ent, origin), nosteporg);
	VectorCopy (ed_vector(ent, velocity), nostepvel);

//
// try moving up and forward to go up a step
//
	VectorCopy (oldorg, ed_vector(ent, origin));	// back to start pos

	VectorCopy (vec3_origin, upmove);
	VectorCopy (vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2]*host_frametime;

// move up
	SV_PushEntity (ent, upmove);	// FIXME: don't link?

// move forward
	ed_vector(ent, velocity)[0] = oldvel[0];
	ed_vector(ent, velocity)[1] = oldvel[1];
	ed_vector(ent, velocity)[2] = 0;
	clip = SV_FlyMove (ent, host_frametime, &steptrace);

// check for stuckness, possibly due to the limited precision of floats
// in the clipping hulls
	if (clip)
	{
		if ( fabs(oldorg[1] - ed_vector(ent, origin)[1]) < 0.03125
		&& fabs(oldorg[0] - ed_vector(ent, origin)[0]) < 0.03125 )
		{	// stepping up didn't make any progress
			clip = SV_TryUnstick (ent, oldvel);
		}
	}
	
// extra friction based on view angle
	if ( clip & 2 )
		SV_WallFriction (ent, &steptrace);

// move down
	downtrace = SV_PushEntity (ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ed_float(ent, solid) == SOLID_BSP)
		{
			ed_float(ent, flags) =	(int)ed_float(ent, flags) | FL_ONGROUND;
			ed_set_edict(ent, groundentity, downtrace.ent);
		}
	}
	else
	{
// if the push down didn't end up on good ground, use the move without
// the step up.  This happens near wall / slope combinations, and can
// cause the player to hop up higher on a slope too steep to climb	
		VectorCopy (nosteporg, ed_vector(ent, origin));
		VectorCopy (nostepvel, ed_vector(ent, velocity));
	}
}


/*
================
SV_Physics_Client

Player character actions
================
*/
void SV_Physics_Client (edict_t	*ent, int num)
{
	if ( ! svs.clients[num-1].active )
		return;		// unconnected slot

//
// call standard client pre-think
//	
	sv_pr_float(time) = sv.time;
	sv_pr_int(self) = EDICT_TO_PROG(ent);
	PR_ExecuteProgram (&sv.pr, sv.pr.global_struct.PlayerPreThink);
	
//
// do a move
//
	SV_CheckVelocity (ent);

//
// decide which move function to call
//
	switch ((int)ed_float(ent, movetype))
	{
	case MOVETYPE_NONE:
		if (!SV_RunThink (ent))
			return;
		break;

	case MOVETYPE_WALK:
		if (!SV_RunThink (ent))
			return;
		if (!SV_CheckWater (ent) && ! ((int)ed_float(ent, flags) & FL_WATERJUMP) )
			SV_AddGravity (ent);
		SV_CheckStuck (ent);
#ifdef QUAKE2
		VectorAdd (ed_vector(ent, velocity), ed_vector(ent, basevelocity), ed_vector(ent, velocity));
#endif
		SV_WalkMove (ent);

#ifdef QUAKE2
		VectorSubtract (ed_vector(ent, velocity), ed_vector(ent, basevelocity), ed_vector(ent, velocity));
#endif
		break;
		
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
		SV_Physics_Toss (ent);
		break;

	case MOVETYPE_FLY:
		if (!SV_RunThink (ent))
			return;
		SV_FlyMove (ent, host_frametime, NULL);
		break;
		
	case MOVETYPE_NOCLIP:
		if (!SV_RunThink (ent))
			return;
		VectorMA (ed_vector(ent, origin), host_frametime, ed_vector(ent, velocity), ed_vector(ent, origin));
		break;
		
	default:
		Sys_Error ("SV_Physics_client: bad movetype %i", (int)ed_float(ent, movetype));
	}

//
// call standard player post-think
//		
	SV_LinkEdict (ent, true);

	sv_pr_float(time) = sv.time;
	sv_pr_int(self) = EDICT_TO_PROG(ent);
	PR_ExecuteProgram (&sv.pr, sv.pr.global_struct.PlayerPostThink);
}

//============================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None (edict_t *ent)
{
// regular thinking
	SV_RunThink (ent);
}

/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity
=============
*/
void SV_Physics_Follow (edict_t *ent)
{
// regular thinking
	SV_RunThink (ent);
	VectorAdd (ed_vector(ed_get_edict(ent, aiment), origin), ed_vector(ent, v_angle), ed_vector(ent, origin));
	SV_LinkEdict (ent, true);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip (edict_t *ent)
{
// regular thinking
	if (!SV_RunThink (ent))
		return;
	
	VectorMA (ed_vector(ent, angles), host_frametime, ed_vector(ent, avelocity), ed_vector(ent, angles));
	VectorMA (ed_vector(ent, origin), host_frametime, ed_vector(ent, velocity), ed_vector(ent, origin));

	SV_LinkEdict (ent, false);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition (edict_t *ent)
{
	int		cont;
	vec3_t	point;
	
	point[0] = ed_vector(ent, origin)[0];
	point[1] = ed_vector(ent, origin)[1];
	point[2] = ed_vector(ent, origin)[2] + ed_vector(ent, mins)[2] + 1;	
	cont = SV_PointContents (point);

	if (!ed_float(ent, watertype))
	{	// just spawned here
		ed_float(ent, watertype) = cont;
		ed_float(ent, waterlevel) = 1;
		return;
	}
	
	if (cont <= CONTENTS_WATER)
	{
		if (ed_float(ent, watertype) == CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}		
		ed_float(ent, watertype) = cont;
		ed_float(ent, waterlevel) = 1;
	}
	else
	{
		if (ed_float(ent, watertype) != CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}		
		ed_float(ent, watertype) = CONTENTS_EMPTY;
		ed_float(ent, waterlevel) = cont;
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss (edict_t *ent)
{
	trace_t	trace;
	vec3_t	move;
	float	backoff;
#ifdef QUAKE2
	edict_t	*groundentity;

	groundentity = ed_get_edict(ent, groundentity);
	if ((int)ed_float(groundentity, flags) & FL_CONVEYOR)
		VectorScale(ed_vector(groundentity, movedir), ed_float(groundentity, speed), ed_vector(ent, basevelocity));
	else
		VectorCopy(vec_origin, ed_vector(ent, basevelocity));
	SV_CheckWater (ent);
#endif
	// regular thinking
	if (!SV_RunThink (ent))
		return;

#ifdef QUAKE2
	if (ed_vector(ent, velocity)[2] > 0)
		ed_float(ent, flags) = (int)ed_float(ent, flags) & ~FL_ONGROUND;

	if ( ((int)ed_float(ent, flags) & FL_ONGROUND) )
//@@
		if (VectorCompare(ed_vector(ent, basevelocity), vec_origin))
			return;

	SV_CheckVelocity (ent);

// add gravity
	if (! ((int)ed_float(ent, flags) & FL_ONGROUND)
		&& ed_float(ent, movetype) != MOVETYPE_FLY
		&& ed_float(ent, movetype) != MOVETYPE_BOUNCEMISSILE
		&& ed_float(ent, movetype) != MOVETYPE_FLYMISSILE)
			SV_AddGravity (ent);

#else
// if onground, return without moving
	if ( ((int)ed_float(ent, flags) & FL_ONGROUND) )
		return;

	SV_CheckVelocity (ent);

// add gravity
	if (ed_float(ent, movetype) != MOVETYPE_FLY
	&& ed_float(ent, movetype) != MOVETYPE_FLYMISSILE)
		SV_AddGravity (ent);
#endif

// move angles
	VectorMA (ed_vector(ent, angles), host_frametime, ed_vector(ent, avelocity), ed_vector(ent, angles));

// move origin
#ifdef QUAKE2
	VectorAdd (ed_vector(ent, velocity), ed_vector(ent, basevelocity), ed_vector(ent, velocity));
#endif
	VectorScale (ed_vector(ent, velocity), host_frametime, move);
	trace = SV_PushEntity (ent, move);
#ifdef QUAKE2
	VectorSubtract (ed_vector(ent, velocity), ed_vector(ent, basevelocity), ed_vector(ent, velocity));
#endif
	if (trace.fraction == 1)
		return;
	if (ent->free)
		return;
	
	if (ed_float(ent, movetype) == MOVETYPE_BOUNCE)
		backoff = 1.5;
	else if (ed_float(ent, movetype) == MOVETYPE_BOUNCEMISSILE)
		backoff = 2.0;
	else
		backoff = 1;

	ClipVelocity (ed_vector(ent, velocity), trace.plane.normal, ed_vector(ent, velocity), backoff);

// stop if on ground
	if (trace.plane.normal[2] > 0.7)
	{		
		/* Slope bounce fix from Spike! */
		if (DotProduct(trace.plane.normal, ed_vector(ent, velocity)) < 60 || (ed_float(ent, movetype) != MOVETYPE_BOUNCE && ed_float(ent, movetype) != MOVETYPE_BOUNCEMISSILE))
		{
			ed_float(ent, flags) = (int)ed_float(ent, flags) | FL_ONGROUND;
			ed_set_edict(ent, groundentity, trace.ent);
			VectorCopy (vec3_origin, ed_vector(ent, velocity));
			VectorCopy (vec3_origin, ed_vector(ent, avelocity));
		}
	}
	
// check for in water
	SV_CheckWaterTransition (ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void SV_Physics_Step (edict_t *ent)
{
	bool	hitsound;

// freefall if not onground
	if ( ! ((int)ed_float(ent, flags) & (FL_ONGROUND | FL_FLY | FL_SWIM) ) )
	{
		if (ed_vector(ent, velocity)[2] < sv_gravity.value*-0.1)
			hitsound = true;
		else
			hitsound = false;

		SV_AddGravity (ent);
		SV_CheckVelocity (ent);
		SV_FlyMove (ent, host_frametime, NULL);
		SV_LinkEdict (ent, true);

		if ( (int)ed_float(ent, flags) & FL_ONGROUND )	// just hit ground
		{
			if (hitsound)
				SV_StartSound (ent, 0, "demon/dland2.wav", 255, 1);
		}
	}

// regular thinking
	SV_RunThink (ent);
	
	SV_CheckWaterTransition (ent);
}

//============================================================================

/*
================
SV_Physics

================
*/
void SV_Physics ()
{
	int		i;
	edict_t	*ent;

// let the progs know that a new frame has started
	sv_pr_int(self) = EDICT_TO_PROG(sv.edicts);
	sv_pr_int(other) = EDICT_TO_PROG(sv.edicts);
	sv_pr_float(time) = sv.time;
	PR_ExecuteProgram (&sv.pr, sv.pr.global_struct.StartFrame);

//SV_CheckAllEnts ();

//
// treat each object in turn
//
	ent = sv.edicts;
	for (i=0 ; i<sv.num_edicts ; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;

		if (sv_pr_float(force_retouch))
		{
			SV_LinkEdict (ent, true);	// force retouch even for stationary
		}

		if (i > 0 && i <= svs.maxclients)
			SV_Physics_Client (ent, i);
		else if (ed_float(ent, movetype) == MOVETYPE_PUSH)
			SV_Physics_Pusher (ent);
		else if (ed_float(ent, movetype) == MOVETYPE_NONE)
			SV_Physics_None (ent);
		else if (ed_float(ent, movetype) == MOVETYPE_FOLLOW)
			SV_Physics_Follow (ent);
		else if (ed_float(ent, movetype) == MOVETYPE_NOCLIP)
			SV_Physics_Noclip (ent);
		else if (ed_float(ent, movetype) == MOVETYPE_STEP)
			SV_Physics_Step (ent);
		else if (ed_float(ent, movetype) == MOVETYPE_TOSS 
			  || ed_float(ent, movetype) == MOVETYPE_BOUNCE
			  || ed_float(ent, movetype) == MOVETYPE_BOUNCEMISSILE
			  || ed_float(ent, movetype) == MOVETYPE_FLY
			  || ed_float(ent, movetype) == MOVETYPE_FLYMISSILE)
			SV_Physics_Toss (ent);
		else
			Sys_Error ("SV_Physics: bad movetype %i", (int)ed_float(ent, movetype));			
	}
	
	if (sv_pr_float(force_retouch))
		sv_pr_float(force_retouch)--;	

	sv.time += host_frametime;
}

