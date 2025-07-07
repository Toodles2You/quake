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

static cvar_t chase_back = {"chase_back", "100"};
static cvar_t chase_up = {"chase_up", "16"};
static cvar_t chase_right = {"chase_right", "0"};
static cvar_t chase_active = {"chase_active", "0"};

void Chase_Init (void)
{
	Cvar_RegisterVariable (src_client, &chase_back);
	Cvar_RegisterVariable (src_client, &chase_up);
	Cvar_RegisterVariable (src_client, &chase_right);
	Cvar_RegisterVariable (src_client, &chase_active);
}

bool Chase_Active (void)
{
	return chase_active.value && Host_IsLocalGame () && !Host_IsMultiplayer ();
}

void Chase_Update (void)
{
	if (!Chase_Active ())
		return;

	int i;
	float dist;
	vec3_t forward, up, right;
	vec3_t dest, stop;
	vec3_t chase_dest;

	// if can't see player, reset
	AngleVectors (cl.viewangles, forward, right, up);

	// calc exact destination
	for (i = 0; i < 3; i++)
		chase_dest[i] = r_refdef.vieworg[i] - forward[i] * chase_back.value - right[i] * chase_right.value;
	chase_dest[2] = r_refdef.vieworg[2] + chase_up.value;

	pmtrace_t trace = PM_TraceLine (r_refdef.vieworg, chase_dest);
	VectorCopy (trace.endpos, chase_dest);

	// find the spot the player is looking at
	VectorMA (r_refdef.vieworg, 4096, forward, dest);
	trace = PM_TraceLine (r_refdef.vieworg, dest);
	VectorCopy (trace.endpos, stop);

	// calculate pitch to look at the same spot from camera
	VectorSubtract (stop, r_refdef.vieworg, stop);
	dist = DotProduct (stop, forward);
	if (dist < 1)
		dist = 1;
	r_refdef.viewangles[PITCH] = -atan (stop[2] / dist) / M_PI * 180;

	// move towards destination
	VectorCopy (chase_dest, r_refdef.vieworg);
}
