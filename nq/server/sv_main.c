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
===============
SV_Init
===============
*/
void SV_Init ()
{
	int i;
	extern cvar_t sv_maxvelocity;
	extern cvar_t sv_gravity;
	extern cvar_t sv_aim;
	extern cvar_t sv_stopspeed;
	extern cvar_t sv_spectatormaxspeed;
	extern cvar_t sv_accelerate;
	extern cvar_t sv_airaccelerate;
	extern cvar_t sv_wateraccelerate;
	extern cvar_t sv_friction;
	extern cvar_t sv_waterfriction;

	extern cvar_t sv_mintic;
	extern cvar_t sv_maxtic;

	Cvar_RegisterVariable (&sv_mintic);
	Cvar_RegisterVariable (&sv_maxtic);

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_spectatormaxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_airaccelerate);
	Cvar_RegisterVariable (&sv_wateraccelerate);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_waterfriction);

	Cvar_RegisterVariable (&sv_aim);

	for (i = 0; i < MAX_MODELS; i++)
	{
		sprintf (localmodels[i], "*%i", i);
	}

	Info_SetValueForStarKey (svs.info, "*version", QUAKE_VERSION, MAX_SERVERINFO_STRING);
}
