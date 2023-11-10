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

#define	RETURN_EDICT(e) (((int32_t *)pr->globals)[OFS_RETURN] = EDICT_TO_PROG(e))

/*
===============================================================================

						BUILT-IN FUNCTIONS

===============================================================================
*/

char *PF_VarString (progs_state_t *pr, int first)
{
	int		i;
	static char out[256];
	
	out[0] = 0;
	for (i=first ; i<pr->argc ; i++)
	{
		strcat (out, pr_get_string(pr, (OFS_PARM0+i*3)));
	}
	return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void PF_error (progs_state_t *pr)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(pr, 0);
	Con_Printf ("======SERVER ERROR in %s:\n%s\n"
	,PR_GetString(pr, pr->xfunction->s_name),s);
	ed = PROG_TO_EDICT(pr_int(pr, self));
	ED_Print (ed);

	Host_Error ("Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror (progs_state_t *pr)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(pr, 0);
	Con_Printf ("======OBJECT ERROR in %s:\n%s\n"
	,PR_GetString(pr, pr->xfunction->s_name),s);
	ed = PROG_TO_EDICT(pr_int(pr, self));
	ED_Print (ed);
	ED_Free (ed);
	
	Host_Error ("Program error");
}



/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors (progs_state_t *pr)
{
	AngleVectors (pr_global_ptr(pr, float, OFS_PARM0), pr_vector(pr, v_forward), pr_vector(pr, v_right), pr_vector(pr, v_up));
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin (progs_state_t *pr)
{
	edict_t	*e;
	float	*org;
	
	e = pr_get_edict(pr, OFS_PARM0);
	org = pr_global_ptr(pr, float, OFS_PARM1);
	VectorCopy (org, ed_vector(e, origin));
	SV_LinkEdict (e, false);
}


void SetMinMaxSize (edict_t *e, float *min, float *max, bool rotate)
{
	float	*angles;
	vec3_t	rmin, rmax;
	float	bounds[2][3];
	float	xvector[2], yvector[2];
	float	a;
	vec3_t	base, transformed;
	int		i, j, k, l;
	float	save;
	
	for (i = 0; i < 3; i++)
	{
		if (min[i] > max[i])
		{
			Con_DPrintf("backwards mins/maxs\n");
			save = min[i];
			min[i] = max[i];
			max[i] = save;
		}
	}

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy (min, rmin);
		VectorCopy (max, rmax);
	}
	else
	{
	// find min / max for rotations
		angles = ed_vector(e, angles);
		
		a = angles[1]/180 * M_PI;
		
		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);
		
		VectorCopy (min, bounds[0]);
		VectorCopy (max, bounds[1]);
		
		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;
		
		for (i=0 ; i<= 1 ; i++)
		{
			base[0] = bounds[i][0];
			for (j=0 ; j<= 1 ; j++)
			{
				base[1] = bounds[j][1];
				for (k=0 ; k<= 1 ; k++)
				{
					base[2] = bounds[k][2];
					
				// transform the point
					transformed[0] = xvector[0]*base[0] + yvector[0]*base[1];
					transformed[1] = xvector[1]*base[0] + yvector[1]*base[1];
					transformed[2] = base[2];
					
					for (l=0 ; l<3 ; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}
	
// set derived values
	VectorCopy (rmin, ed_vector(e, mins));
	VectorCopy (rmax, ed_vector(e, maxs));
	VectorSubtract (max, min, ed_vector(e, size));
	
	SV_LinkEdict (e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize (progs_state_t *pr)
{
	edict_t	*e;
	float	*min, *max;
	
	e = pr_get_edict(pr, OFS_PARM0);
	min = pr_global_ptr(pr, float, OFS_PARM1);
	max = pr_global_ptr(pr, float, OFS_PARM2);
	SetMinMaxSize (e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel (progs_state_t *pr)
{
	edict_t	*e;
	char	*m, **check;
	cmodel_t	*mod = NULL;
	int		i = 0;

	e = pr_get_edict(pr, OFS_PARM0);
	m = pr_get_string(pr, OFS_PARM1);

// check to see if model was properly precached
	if (m && m[0] > ' ')
	{
		for (i=0, check = sv.model_precache ; *check ; i++, check++)
			if (!strcmp(*check, m))
				break;
				
		if (!*check)
			PR_RunError (pr, "no precache: %s\n", m);

		mod = sv.models[i];
	}

	ed_set_string(e, model, m);
	ed_float(e, modelindex) = i;
	
	if (mod)
		SetMinMaxSize (e, mod->mins, mod->maxs, true);
	else
		SetMinMaxSize (e, vec3_origin, vec3_origin, true);
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint (progs_state_t *pr)
{
	char		*s;

	s = PF_VarString(pr, 0);
	SV_BroadcastPrintf ("%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint (progs_state_t *pr)
{
	char		*s;
	client_t	*client;
	int			entnum;
	
	entnum = pr_get_edict_num(pr, OFS_PARM0);
	s = PF_VarString(pr, 1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_print);
	MSG_WriteString (&client->message, s );
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint (progs_state_t *pr)
{
	char		*s;
	client_t	*client;
	int			entnum;
	
	entnum = pr_get_edict_num(pr, OFS_PARM0);
	s = PF_VarString(pr, 1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_centerprint);
	MSG_WriteString (&client->message, s );
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize (progs_state_t *pr)
{
	float	*value1;
	vec3_t	newvalue;
	float	new;
	
	value1 = pr_global_ptr(pr, float, OFS_PARM0);

	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	new = sqrt(new);
	
	if (new == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		new = 1/new;
		newvalue[0] = value1[0] * new;
		newvalue[1] = value1[1] * new;
		newvalue[2] = value1[2] * new;
	}
	
	VectorCopy (newvalue, pr_global_ptr(pr, float, OFS_RETURN));	
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen (progs_state_t *pr)
{
	float	*value1;
	float	new;
	
	value1 = pr_global_ptr(pr, float, OFS_PARM0);

	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	new = sqrt(new);
	
	pr_global(pr, float, OFS_RETURN) = new;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw (progs_state_t *pr)
{
	float	*value1;
	float	yaw;
	
	value1 = pr_global_ptr(pr, float, OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	pr_global(pr, float, OFS_RETURN) = yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles (progs_state_t *pr)
{
	float	*value1;
	float	forward;
	float	yaw, pitch;
	
	value1 = pr_global_ptr(pr, float, OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	pr_global(pr, float, OFS_RETURN+0) = pitch;
	pr_global(pr, float, OFS_RETURN+1) = yaw;
	pr_global(pr, float, OFS_RETURN+2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random (progs_state_t *pr)
{
	float		num;
		
	num = (rand ()&0x7fff) / ((float)0x7fff);
	
	pr_global(pr, float, OFS_RETURN) = num;
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle (progs_state_t *pr)
{
	float		*org, *dir;
	float		color;
	float		count;
			
	org = pr_global_ptr(pr, float, OFS_PARM0);
	dir = pr_global_ptr(pr, float, OFS_PARM1);
	color = pr_global(pr, float, OFS_PARM2);
	count = pr_global(pr, float, OFS_PARM3);
	SV_StartParticle (org, dir, color, count);
}


/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound (progs_state_t *pr)
{
	char		**check;
	char		*samp;
	float		*pos;
	float 		vol, attenuation;
	int			i, soundnum;

	pos = pr_global_ptr(pr, float, OFS_PARM0);			
	samp = pr_get_string(pr, OFS_PARM1);
	vol = pr_global(pr, float, OFS_PARM2);
	attenuation = pr_global(pr, float, OFS_PARM3);
	
// check to see if samp was properly precached
	for (soundnum=0, check = sv.sound_precache ; *check ; check++, soundnum++)
		if (!strcmp(*check,samp))
			break;
			
	if (!*check)
	{
		Con_Printf ("no precache: %s\n", samp);
		return;
	}

// add an svc_spawnambient command to the level signon packet

	MSG_WriteByte (&sv.signon,svc_spawnstaticsound);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord(&sv.signon, pos[i]);

	MSG_WriteByte (&sv.signon, soundnum);

	MSG_WriteByte (&sv.signon, vol*255);
	MSG_WriteByte (&sv.signon, attenuation*64);

}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound (progs_state_t *pr)
{
	char		*sample;
	int			channel;
	edict_t		*entity;
	int 		volume;
	float attenuation;
		
	entity = pr_get_edict(pr, OFS_PARM0);
	channel = pr_global(pr, float, OFS_PARM1);
	sample = pr_get_string(pr, OFS_PARM2);
	volume = pr_global(pr, float, OFS_PARM3) * 255;
	attenuation = pr_global(pr, float, OFS_PARM4);
	
	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	SV_StartSound (entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void PF_break (progs_state_t *pr)
{
Con_Printf ("break statement\n");
*(int32_t *)-4 = 0;	// dump to debugger
//	PR_RunError (pr, "break statement");
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline (progs_state_t *pr)
{
	float	*v1, *v2;
	trace_t	trace;
	int		nomonsters;
	edict_t	*ent;

	v1 = pr_global_ptr(pr, float, OFS_PARM0);
	v2 = pr_global_ptr(pr, float, OFS_PARM1);
	nomonsters = pr_global(pr, float, OFS_PARM2);
	ent = pr_get_edict(pr, OFS_PARM3);

	trace = SV_Move (v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

	pr_float(pr, trace_allsolid) = trace.allsolid;
	pr_float(pr, trace_startsolid) = trace.startsolid;
	pr_float(pr, trace_fraction) = trace.fraction;
	pr_float(pr, trace_inwater) = trace.inwater;
	pr_float(pr, trace_inopen) = trace.inopen;
	VectorCopy (trace.endpos, pr_vector(pr, trace_endpos));
	VectorCopy (trace.plane.normal, pr_vector(pr, trace_plane_normal));
	pr_float(pr, trace_plane_dist) =  trace.plane.dist;	
	if (trace.ent)
		pr_int(pr, trace_ent) = EDICT_TO_PROG(trace.ent);
	else
		pr_int(pr, trace_ent) = EDICT_TO_PROG(sv.edicts);
}


/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos (progs_state_t *pr)
{
}

//============================================================================

byte	checkpvs[MAX_MAP_LEAFS/8];

int PF_newcheckclient (int check)
{
	int		i;
	byte	*pvs;
	edict_t	*ent;
	mleaf_t	*leaf;
	vec3_t	org;

// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > svs.maxclients)
		check = svs.maxclients;

	if (check == svs.maxclients)
		i = 1;
	else
		i = check + 1;

	for ( ;  ; i++)
	{
		if (i == svs.maxclients+1)
			i = 1;

		ent = EDICT_NUM(i);

		if (i == check)
			break;	// didn't find anything else

		if (ent->free)
			continue;
		if (ed_float(ent, health) <= 0)
			continue;
		if ((int)ed_float(ent, flags) & FL_NOTARGET)
			continue;

	// anything that is a client, or has a client as an enemy
		break;
	}

// get the PVS for the entity
	VectorAdd (ed_vector(ent, origin), ed_vector(ent, view_ofs), org);
	leaf = CMod_PointInLeaf (org, sv.worldmodel);
	pvs = CMod_LeafPVS (leaf, sv.worldmodel);
	memcpy (checkpvs, pvs, (sv.worldmodel->numleafs+7)>>3 );

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define	MAX_CHECK	16
int c_invis, c_notvis;
void PF_checkclient (progs_state_t *pr)
{
	edict_t	*ent, *self;
	mleaf_t	*leaf;
	int		l;
	vec3_t	view;
	
// find a new check if on a new frame
	if (sv.time - sv.lastchecktime >= 0.1)
	{
		sv.lastcheck = PF_newcheckclient (sv.lastcheck);
		sv.lastchecktime = sv.time;
	}

// return check if it might be visible	
	ent = EDICT_NUM(sv.lastcheck);
	if (ent->free || ed_float(ent, health) <= 0)
	{
		RETURN_EDICT(sv.edicts);
		return;
	}

// if current entity can't possibly see the check entity, return 0
	self = PROG_TO_EDICT(pr_int(pr, self));
	VectorAdd (ed_vector(self, origin), ed_vector(self, view_ofs), view);
	leaf = CMod_PointInLeaf (view, sv.worldmodel);
	l = (leaf - sv.worldmodel->leafs) - 1;
	if ( (l<0) || !(checkpvs[l>>3] & (1<<(l&7)) ) )
	{
c_notvis++;
		RETURN_EDICT(sv.edicts);
		return;
	}

// might be able to see it
c_invis++;
	RETURN_EDICT(ent);
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd (progs_state_t *pr)
{
	int		entnum;
	char	*str;
	client_t	*old;
	
	entnum = pr_get_edict_num(pr, OFS_PARM0);
	if (entnum < 1 || entnum > svs.maxclients)
		PR_RunError (pr, "Parm 0 not a client");
	str = pr_get_string(pr, OFS_PARM1);	
	
	old = host_client;
	host_client = &svs.clients[entnum-1];
	Host_ClientCommands ("%s", str);
	host_client = old;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd (progs_state_t *pr)
{
	char	*str;
	
	str = pr_get_string(pr, OFS_PARM0);	
	Cbuf_AddText (str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void PF_cvar (progs_state_t *pr)
{
	char	*str;
	
	str = pr_get_string(pr, OFS_PARM0);
	
	pr_global(pr, float, OFS_RETURN) = Cvar_VariableValue (str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set (progs_state_t *pr)
{
	char	*var, *val;
	
	var = pr_get_string(pr, OFS_PARM0);
	val = pr_get_string(pr, OFS_PARM1);
	
	Cvar_Set (var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius (progs_state_t *pr)
{
	edict_t	*ent, *chain;
	float	rad;
	float	*org;
	vec3_t	eorg;
	int		i, j;

	chain = (edict_t *)sv.edicts;
	
	org = pr_global_ptr(pr, float, OFS_PARM0);
	rad = pr_global(pr, float, OFS_PARM1);

	ent = NEXT_EDICT(sv.edicts);
	for (i=1 ; i<sv.num_edicts ; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;
		if (ed_float(ent, solid) == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ed_vector(ent, origin)[j] + (ed_vector(ent, mins)[j] + ed_vector(ent, maxs)[j])*0.5);			
		if (Length(eorg) > rad)
			continue;
			
		ed_set_edict(ent, chain, chain);
		chain = ent;
	}

	RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
void PF_dprint (progs_state_t *pr)
{
	Con_DPrintf ("%s",PF_VarString(pr, 0));
}

char	pr_string_temp[128];

void PF_ftos (progs_state_t *pr)
{
	float	v;
	v = pr_global(pr, float, OFS_PARM0);
	
	if (v == (int)v)
		sprintf (pr_string_temp, "%d",(int)v);
	else
		sprintf (pr_string_temp, "%5.1f",v);
	pr_set_string(pr, OFS_RETURN, pr_string_temp);
}

void PF_fabs (progs_state_t *pr)
{
	float	v;
	v = pr_global(pr, float, OFS_PARM0);
	pr_global(pr, float, OFS_RETURN) = fabs(v);
}

void PF_vtos (progs_state_t *pr)
{
	sprintf (pr_string_temp, "'%5.1f %5.1f %5.1f'", pr_global_ptr(pr, float, OFS_PARM0)[0], pr_global_ptr(pr, float, OFS_PARM0)[1], pr_global_ptr(pr, float, OFS_PARM0)[2]);
	pr_set_string(pr, OFS_RETURN, pr_string_temp);
}

void PF_Spawn (progs_state_t *pr)
{
	edict_t	*ed;
	ed = ED_Alloc();
	RETURN_EDICT(ed);
}

void PF_Remove (progs_state_t *pr)
{
	edict_t	*ed;
	
	ed = pr_get_edict(pr, OFS_PARM0);
	ED_Free (ed);
}


// entity (entity start, .string field, string match) find = #5;
void PF_Find (progs_state_t *pr)
{
	int		e;	
	int		f;
	char	*s, *t;
	edict_t	*ed;

	e = pr_get_edict_num(pr, OFS_PARM0);
	f = pr_global(pr, int, OFS_PARM1);
	s = pr_get_string(pr, OFS_PARM2);
	if (!s)
		PR_RunError (pr, "PF_Find: bad search string");
		
	for (e++ ; e < sv.num_edicts ; e++)
	{
		ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		t = PR_GetString(pr, *(string_t *)((float *)(ed + 1) + f));
		if (!t)
			continue;
		if (!strcmp(t,s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.edicts);
}

void PR_CheckEmptyString (progs_state_t *pr, char *s)
{
	if (s[0] <= ' ')
		PR_RunError (pr, "Bad string");
}

void PF_precache_file (progs_state_t *pr)
{	// precache_file is only used to copy files with qcc, it does nothing
	pr_global(pr, int, OFS_RETURN) = pr_global(pr, int, OFS_PARM0);
}

void PF_precache_sound (progs_state_t *pr)
{
	char	*s;
	int		i;
	
	if (sv.state != ss_loading)
		PR_RunError (pr, "PF_Precache_*: Precache can only be done in spawn functions");
		
	s = pr_get_string(pr, OFS_PARM0);
	pr_global(pr, int, OFS_RETURN) = pr_global(pr, int, OFS_PARM0);
	PR_CheckEmptyString (pr, s);
	
	for (i=0 ; i<MAX_SOUNDS ; i++)
	{
		if (!sv.sound_precache[i])
		{
			sv.sound_precache[i] = s;
			return;
		}
		if (!strcmp(sv.sound_precache[i], s))
			return;
	}
	PR_RunError (pr, "PF_precache_sound: overflow");
}

void PF_precache_model (progs_state_t *pr)
{
	char	*s;
	int		i;
	
	if (sv.state != ss_loading)
		PR_RunError (pr, "PF_Precache_*: Precache can only be done in spawn functions");
		
	s = pr_get_string(pr, OFS_PARM0);
	pr_global(pr, int, OFS_RETURN) = pr_global(pr, int, OFS_PARM0);
	PR_CheckEmptyString (pr, s);

	for (i=0 ; i<MAX_MODELS ; i++)
	{
		if (!sv.model_precache[i])
		{
			sv.model_precache[i] = s;
			sv.models[i] = CMod_ForName (s, true, false);
			return;
		}
		if (!strcmp(sv.model_precache[i], s))
			return;
	}
	PR_RunError (pr, "PF_precache_model: overflow");
}


void PF_coredump (progs_state_t *pr)
{
	ED_PrintEdicts ();
}

void PF_traceon (progs_state_t *pr)
{
	pr_trace = true;
}

void PF_traceoff (progs_state_t *pr)
{
	pr_trace = false;
}

void PF_eprint (progs_state_t *pr)
{
	ED_PrintNum (pr_get_edict_num(pr, OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove (progs_state_t *pr)
{
	edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	dfunction_t	*oldf;
	int 	oldself;
	
	ent = PROG_TO_EDICT(pr_int(pr, self));
	yaw = pr_global(pr, float, OFS_PARM0);
	dist = pr_global(pr, float, OFS_PARM1);
	
	if ( !( (int)ed_float(ent, flags) & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		pr_global(pr, float, OFS_RETURN) = 0;
		return;
	}

	yaw = yaw*M_PI*2 / 360;
	
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because SV_movestep may call other progs
	oldf = pr->xfunction;
	oldself = pr_int(pr, self);
	
	pr_global(pr, float, OFS_RETURN) = SV_movestep(ent, move, true);
	
	
// restore program state
	pr->xfunction = oldf;
	pr_int(pr, self) = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor (progs_state_t *pr)
{
	edict_t		*ent;
	vec3_t		end;
	trace_t		trace;
	
	ent = PROG_TO_EDICT(pr_int(pr, self));

	VectorCopy (ed_vector(ent, origin), end);
	end[2] -= 256;
	
	trace = SV_Move (ed_vector(ent, origin), ed_vector(ent, mins), ed_vector(ent, maxs), end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
		pr_global(pr, float, OFS_RETURN) = 0;
	else
	{
		VectorCopy (trace.endpos, ed_vector(ent, origin));
		SV_LinkEdict (ent, false);
		ed_float(ent, flags) = (int)ed_float(ent, flags) | FL_ONGROUND;
		ed_set_edict(ent, groundentity, trace.ent);
		pr_global(pr, float, OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle (progs_state_t *pr)
{
	int		style;
	char	*val;
	client_t	*client;
	int			j;
	
	style = pr_global(pr, float, OFS_PARM0);
	val = pr_get_string(pr, OFS_PARM1);

// change the string in sv
	sv.lightstyles[style] = val;
	
// send message to all clients on this server
	if (sv.state != ss_active)
		return;
	
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
		if (client->active || client->spawned)
		{
			MSG_WriteChar (&client->message, svc_lightstyle);
			MSG_WriteChar (&client->message,style);
			MSG_WriteString (&client->message, val);
		}
}

void PF_rint (progs_state_t *pr)
{
	float	f;
	f = pr_global(pr, float, OFS_PARM0);
	if (f > 0)
		pr_global(pr, float, OFS_RETURN) = (int)(f + 0.5);
	else
		pr_global(pr, float, OFS_RETURN) = (int)(f - 0.5);
}
void PF_floor (progs_state_t *pr)
{
	pr_global(pr, float, OFS_RETURN) = floor(pr_global(pr, float, OFS_PARM0));
}
void PF_ceil (progs_state_t *pr)
{
	pr_global(pr, float, OFS_RETURN) = ceil(pr_global(pr, float, OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom (progs_state_t *pr)
{
	edict_t	*ent;
	
	ent = pr_get_edict(pr, OFS_PARM0);

	pr_global(pr, float, OFS_RETURN) = SV_CheckBottom (ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents (progs_state_t *pr)
{
	float	*v;
	
	v = pr_global_ptr(pr, float, OFS_PARM0);

	pr_global(pr, float, OFS_RETURN) = SV_PointContents (v);	
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent (progs_state_t *pr)
{
	int		i;
	edict_t	*ent;
	
	i = pr_get_edict_num(pr, OFS_PARM0);
	while (1)
	{
		i++;
		if (i == sv.num_edicts)
		{
			RETURN_EDICT(sv.edicts);
			return;
		}
		ent = EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
cvar_t	sv_aim = {"sv_aim", "0.93"};
void PF_aim (progs_state_t *pr)
{
	edict_t	*ent, *check, *bestent;
	vec3_t	start, dir, end, bestdir;
	int		i, j;
	trace_t	tr;
	float	dist, bestdist;
	float	speed;
	
	ent = pr_get_edict(pr, OFS_PARM0);
	speed = pr_global(pr, float, OFS_PARM1);

	VectorCopy (ed_vector(ent, origin), start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy (pr_vector(pr, v_forward), dir);
	VectorMA (start, 2048, dir, end);
	tr = SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.ent && ed_float(tr.ent, takedamage) == DAMAGE_AIM
	&& (!teamplay.value || ed_float(ent, team) <=0 || ed_float(ent, team) != ed_float(tr.ent, team)) )
	{
		VectorCopy (pr_vector(pr, v_forward), pr_global_ptr(pr, float, OFS_RETURN));
		return;
	}


// try all possible entities
	VectorCopy (dir, bestdir);
	bestdist = sv_aim.value;
	bestent = NULL;
	
	check = NEXT_EDICT(sv.edicts);
	for (i=1 ; i<sv.num_edicts ; i++, check = NEXT_EDICT(check) )
	{
		if (ed_float(check, takedamage) != DAMAGE_AIM)
			continue;
		if (check == ent)
			continue;
		if (teamplay.value && ed_float(ent, team) > 0 && ed_float(ent, team) == ed_float(check, team))
			continue;	// don't aim at teammate
		for (j=0 ; j<3 ; j++)
			end[j] = ed_vector(check, origin)[j]
			+ 0.5*(ed_vector(check, mins)[j] + ed_vector(check, maxs)[j]);
		VectorSubtract (end, start, dir);
		VectorNormalize (dir);
		dist = DotProduct (dir, pr_vector(pr, v_forward));
		if (dist < bestdist)
			continue;	// to far to turn
		tr = SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
		if (tr.ent == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}
	
	if (bestent)
	{
		VectorSubtract (ed_vector(bestent, origin), ed_vector(ent, origin), dir);
		dist = DotProduct (dir, pr_vector(pr, v_forward));
		VectorScale (pr_vector(pr, v_forward), dist, end);
		end[2] = dir[2];
		VectorNormalize (end);
		VectorCopy (end, pr_global_ptr(pr, float, OFS_RETURN));	
	}
	else
	{
		VectorCopy (bestdir, pr_global_ptr(pr, float, OFS_RETURN));
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw (progs_state_t *pr)
{
	edict_t		*ent;
	float		ideal, current, move, speed;
	
	ent = PROG_TO_EDICT(pr_int(pr, self));
	current = anglemod(ed_vector(ent, angles)[1]);
	ideal = ed_float(ent, ideal_yaw);
	speed = ed_float(ent, yaw_speed);
	
	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ed_vector(ent, angles)[1] = anglemod(current + move);
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string

sizebuf_t *WriteDest (progs_state_t *pr)
{
	int		entnum;
	int		dest;
	edict_t	*ent;

	dest = pr_global(pr, float, OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.datagram;
	
	case MSG_ONE:
		ent = PROG_TO_EDICT(pr_int(pr, msg_entity));
		entnum = NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.maxclients)
			PR_RunError (pr, "WriteDest: not a client");
		return &svs.clients[entnum-1].message;
		
	case MSG_ALL:
		return &sv.reliable_datagram;
	
	case MSG_INIT:
		return &sv.signon;

	default:
		PR_RunError (pr, "WriteDest: bad destination");
		break;
	}
	
	return NULL;
}

void PF_WriteByte (progs_state_t *pr)
{
	MSG_WriteByte (WriteDest(pr), pr_global(pr, float, OFS_PARM1));
}

void PF_WriteChar (progs_state_t *pr)
{
	MSG_WriteChar (WriteDest(pr), pr_global(pr, float, OFS_PARM1));
}

void PF_WriteShort (progs_state_t *pr)
{
	MSG_WriteShort (WriteDest(pr), pr_global(pr, float, OFS_PARM1));
}

void PF_WriteLong (progs_state_t *pr)
{
	MSG_WriteLong (WriteDest(pr), pr_global(pr, float, OFS_PARM1));
}

void PF_WriteAngle (progs_state_t *pr)
{
	MSG_WriteAngle (WriteDest(pr), pr_global(pr, float, OFS_PARM1));
}

void PF_WriteCoord (progs_state_t *pr)
{
	MSG_WriteCoord (WriteDest(pr), pr_global(pr, float, OFS_PARM1));
}

void PF_WriteString (progs_state_t *pr)
{
	MSG_WriteString (WriteDest(pr), pr_get_string(pr, OFS_PARM1));
}


void PF_WriteEntity (progs_state_t *pr)
{
	MSG_WriteShort (WriteDest(pr), pr_get_edict_num(pr, OFS_PARM1));
}

//=============================================================================

int SV_ModelIndex (char *name);

void PF_makestatic (progs_state_t *pr)
{
	edict_t	*ent;
	int		i;
	
	ent = pr_get_edict(pr, OFS_PARM0);

	MSG_WriteByte (&sv.signon, svc_spawnstatic);

	MSG_WriteByte (&sv.signon, SV_ModelIndex(ed_get_string(ent, model)));

	MSG_WriteByte (&sv.signon, ed_float(ent, frame));
	MSG_WriteByte (&sv.signon, ed_float(ent, colormap));
	MSG_WriteByte (&sv.signon, ed_float(ent, skin));
	for (i=0 ; i<3 ; i++)
	{
		MSG_WriteCoord(&sv.signon, ed_vector(ent, origin)[i]);
		MSG_WriteAngle(&sv.signon, ed_vector(ent, angles)[i]);
	}

// throw the entity away now
	ED_Free (ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms (progs_state_t *pr)
{
	edict_t	*ent;
	int		i;
	client_t	*client;

	ent = pr_get_edict(pr, OFS_PARM0);
	i = NUM_FOR_EDICT(ent);
	if (i < 1 || i > svs.maxclients)
		PR_RunError (pr, "Entity is not a client");

	// copy spawn parms out of the client_t
	client = svs.clients + (i-1);

	for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
		(&pr_float(pr, parm1))[i] = client->spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel (progs_state_t *pr)
{
	char	*s1;
#ifdef QUAKE2
	char	*s2;
#endif

	if (svs.changelevel_issued)
		return;
	svs.changelevel_issued = true;

	s1 = pr_get_string(pr, OFS_PARM0);
#ifdef QUAKE2
	s2 = pr_get_string(pr, OFS_PARM1);

	if ((int)pr_float(pr, serverflags) & (SFL_NEW_UNIT | SFL_NEW_EPISODE))
		Cbuf_AddText (va("changelevel %s %s\n",s1, s2));
	else
		Cbuf_AddText (va("changelevel2 %s %s\n",s1, s2));
#else
	Cbuf_AddText (va("changelevel %s\n",s1));
#endif
}

void PF_sin (progs_state_t *pr)
{
	pr_global(pr, float, OFS_RETURN) = sinf(pr_global(pr, float, OFS_PARM0));
}

void PF_cos (progs_state_t *pr)
{
	pr_global(pr, float, OFS_RETURN) = cosf(pr_global(pr, float, OFS_PARM0));
}

void PF_sqrt (progs_state_t *pr)
{
	pr_global(pr, float, OFS_RETURN) = sqrtf(pr_global(pr, float, OFS_PARM0));
}

void PF_Fixme (progs_state_t *pr)
{
	PR_RunError (pr, "unimplemented bulitin");
}



builtin_t pr_builtin[] =
{
PF_Fixme,
PF_makevectors,	// void(entity e)	makevectors 		= #1;
PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
PF_setmodel,	// void(entity e, string m) setmodel	= #3;
PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
PF_Fixme,	// void(entity e, vector min, vector max) setabssize = #5;
PF_break,	// void() break						= #6;
PF_random,	// float() random						= #7;
PF_sound,	// void(entity e, float chan, string samp) sound = #8;
PF_normalize,	// vector(vector v) normalize			= #9;
PF_error,	// void(string e) error				= #10;
PF_objerror,	// void(string e) objerror				= #11;
PF_vlen,	// float(vector v) vlen				= #12;
PF_vectoyaw,	// float(vector v) vectoyaw		= #13;
PF_Spawn,	// entity() spawn						= #14;
PF_Remove,	// void(entity e) remove				= #15;
PF_traceline,	// float(vector v1, vector v2, float tryents) traceline = #16;
PF_checkclient,	// entity() clientlist					= #17;
PF_Find,	// entity(entity start, .string fld, string match) find = #18;
PF_precache_sound,	// void(string s) precache_sound		= #19;
PF_precache_model,	// void(string s) precache_model		= #20;
PF_stuffcmd,	// void(entity client, string s)stuffcmd = #21;
PF_findradius,	// entity(vector org, float rad) findradius = #22;
PF_bprint,	// void(string s) bprint				= #23;
PF_sprint,	// void(entity client, string s) sprint = #24;
PF_dprint,	// void(string s) dprint				= #25;
PF_ftos,	// void(string s) ftos				= #26;
PF_vtos,	// void(string s) vtos				= #27;
PF_coredump,
PF_traceon,
PF_traceoff,
PF_eprint,	// void(entity e) debug print an entire entity
PF_walkmove, // float(float yaw, float dist) walkmove
PF_Fixme, // float(float yaw, float dist) walkmove
PF_droptofloor,
PF_lightstyle,
PF_rint,
PF_floor,
PF_ceil,
PF_Fixme,
PF_checkbottom,
PF_pointcontents,
PF_Fixme,
PF_fabs,
PF_aim,
PF_cvar,
PF_localcmd,
PF_nextent,
PF_particle,
PF_changeyaw,
PF_Fixme,
PF_vectoangles,

PF_WriteByte,
PF_WriteChar,
PF_WriteShort,
PF_WriteLong,
PF_WriteCoord,
PF_WriteAngle,
PF_WriteString,
PF_WriteEntity,

PF_sin,
PF_cos,
PF_sqrt,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,

SV_MoveToGoal,
PF_precache_file,
PF_makestatic,

PF_changelevel,
PF_Fixme,

PF_cvar_set,
PF_centerprint,

PF_ambientsound,

PF_precache_model,
PF_precache_sound,		// precache_sound2 is different only for qcc
PF_precache_file,

PF_setspawnparms
};

builtin_t *pr_builtins = pr_builtin;
int pr_numbuiltins = lengthof(pr_builtin);

