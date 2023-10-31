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

static const int pr_type_size[ev_types] =
{
	1,										// ev_void
	sizeof(string_t)	/ sizeof(uint32_t),	// ev_string
	sizeof(float)		/ sizeof(uint32_t),	// ev_float
	sizeof(float) * 3	/ sizeof(uint32_t),	// ev_vector
	sizeof(uint32_t)	/ sizeof(uint32_t),	// ev_entity
	sizeof(uint32_t)	/ sizeof(uint32_t),	// ev_field
	sizeof(func_t)		/ sizeof(uint32_t),	// ev_function
	sizeof(void *)		/ sizeof(uint32_t),	// ev_pointer
};

bool	ED_ParseEpair (void *base, ddef_t *key, char *s);

cvar_t	nomonsters = {"nomonsters", "0"};
cvar_t	gamecfg = {"gamecfg", "0"};
cvar_t	scratch1 = {"scratch1", "0"};
cvar_t	scratch2 = {"scratch2", "0"};
cvar_t	scratch3 = {"scratch3", "0"};
cvar_t	scratch4 = {"scratch4", "0"};
cvar_t	savedgamecfg = {"savedgamecfg", "0", true};
cvar_t	saved1 = {"saved1", "0", true};
cvar_t	saved2 = {"saved2", "0", true};
cvar_t	saved3 = {"saved3", "0", true};
cvar_t	saved4 = {"saved4", "0", true};

#define	MAX_FIELD_LEN	64
#define GEFV_CACHESIZE	2

typedef struct {
	ddef_t	*pcache;
	char	field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache	gefvCache[GEFV_CACHESIZE] = {{NULL, ""}, {NULL, ""}};

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (edict_t *e)
{
	memset (e + 1, 0, sv.pr.progs->entityfields * sizeof(int32_t));
	e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *ED_Alloc ()
{
	int			i;
	edict_t		*e;

	for ( i=svs.maxclients+1 ; i<sv.num_edicts ; i++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			return e;
		}
	}
	
	if (i == MAX_EDICTS)
		Sys_Error ("ED_Alloc: no free edicts");
		
	sv.num_edicts++;
	e = EDICT_NUM(i);
	ED_ClearEdict (e);

	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (edict_t *ed)
{
	SV_UnlinkEdict (ed);		// unlink from world bsp

	ed->free = true;
	ed_int(ed, model) = 0;
	ed_float(ed, takedamage) = 0;
	ed_float(ed, modelindex) = 0;
	ed_float(ed, colormap) = 0;
	ed_float(ed, skin) = 0;
	ed_float(ed, frame) = 0;
	VectorCopy (vec3_origin, ed_vector(ed, origin));
	VectorCopy (vec3_origin, ed_vector(ed, angles));
	ed_float(ed, nextthink) = -1;
	ed_float(ed, solid) = 0;
	
	ed->freetime = sv.time;
}


eval_t *GetEdictFieldValue(edict_t *ed, char *field)
{
	ddef_t			*def = NULL;
	int				i;
	static int		rep = 0;

	for (i=0 ; i<GEFV_CACHESIZE ; i++)
	{
		if (!strcmp(field, gefvCache[i].field))
		{
			def = gefvCache[i].pcache;
			goto Done;
		}
	}

	def = PR_FindField (&sv.pr, field);

	if (strlen(field) < MAX_FIELD_LEN)
	{
		gefvCache[rep].pcache = def;
		strcpy (gefvCache[rep].field, field);
		rep ^= 1;
	}

Done:
	if (!def)
		return NULL;

	return (eval_t *)((int32_t *)(ed + 1) + def->ofs);
}

/*
=============
ED_Print

For debugging
=============
*/
void ED_Print (edict_t *ed)
{
	int		l;
	ddef_t	*d;
	int32_t	*v;
	int		i, j;
	char	*name;
	int		type;

	if (ed->free)
	{
		Con_Printf ("FREE\n");
		return;
	}

	Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
	for (i=1 ; i<sv.pr.progs->numfielddefs ; i++)
	{
		d = &sv.pr.fielddefs[i];
		name = PR_GetString(&sv.pr, d->s_name);
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
			
		v = (int32_t *)((int32_t *)(ed + 1) + d->ofs);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		
		for (j=0 ; j<pr_type_size[type] ; j++)
			if (v[j])
				break;
		if (j == pr_type_size[type])
			continue;
	
		Con_Printf ("%s",name);
		l = strlen (name);
		while (l++ < 15)
			Con_Printf (" ");

		Con_Printf ("%s\n", PR_ValueString(&sv.pr, d->type, (eval_t *)v));		
	}
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (FILE *f, edict_t *ed)
{
	ddef_t	*d;
	int32_t	*v;
	int		i, j;
	char	*name;
	int		type;

	fprintf (f, "{\n");

	if (ed->free)
	{
		fprintf (f, "}\n");
		return;
	}
	
	for (i=1 ; i<sv.pr.progs->numfielddefs ; i++)
	{
		d = &sv.pr.fielddefs[i];
		name = PR_GetString(&sv.pr, d->s_name);
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
			
		v = (int32_t *)((int32_t *)(ed + 1) + d->ofs);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j=0 ; j<pr_type_size[type] ; j++)
			if (v[j])
				break;
		if (j == pr_type_size[type])
			continue;
	
		fprintf (f,"\"%s\" ",name);
		fprintf (f,"\"%s\"\n", PR_UglyValueString(&sv.pr, d->type, (eval_t *)v));		
	}

	fprintf (f, "}\n");
}

void ED_PrintNum (int ent)
{
	ED_Print (EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts ()
{
	int		i;
	
	Con_Printf ("%i entities\n", sv.num_edicts);
	for (i=0 ; i<sv.num_edicts ; i++)
		ED_PrintNum (i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f ()
{
	int		i;
	
	i = atoi (Cmd_Argv(1));
	if (i >= sv.num_edicts)
	{
		Con_Printf("Bad edict number\n");
		return;
	}
	ED_PrintNum (i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count ()
{
	int		i;
	edict_t	*ent;
	int		active, models, solid, step;

	active = models = solid = step = 0;
	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (ed_float(ent, solid))
			solid++;
		if (ed_int(ent, model))
			models++;
		if (ed_float(ent, movetype) == MOVETYPE_STEP)
			step++;
	}

	Con_Printf ("num_edicts:%3i\n", sv.num_edicts);
	Con_Printf ("active    :%3i\n", active);
	Con_Printf ("view      :%3i\n", models);
	Con_Printf ("touch     :%3i\n", solid);
	Con_Printf ("step      :%3i\n", step);

}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals (FILE *f)
{
	ddef_t		*def;
	int			i;
	char		*name;
	int			type;

	fprintf (f,"{\n");
	for (i=0 ; i<sv.pr.progs->numglobaldefs ; i++)
	{
		def = &sv.pr.globaldefs[i];
		type = def->type;
		if ( !(def->type & DEF_SAVEGLOBAL) )
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string
		&& type != ev_float
		&& type != ev_entity)
			continue;

		name = PR_GetString(&sv.pr, def->s_name);		
		fprintf (f,"\"%s\" ", name);
		fprintf (f,"\"%s\"\n", PR_UglyValueString(&sv.pr, type, (eval_t *)&sv.pr.globals[def->ofs]));		
	}
	fprintf (f,"}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals (char *data)
{
	char	keyname[64];
	ddef_t	*key;

	while (1)
	{	
	// parse key
		data = COM_Parse (data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		strcpy (keyname, com_token);

	// parse value	
		data = COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		key = PR_FindGlobal (&sv.pr, keyname);
		if (!key)
		{
			Con_Printf ("'%s' is not a global\n", keyname);
			continue;
		}

		if (!ED_ParseEpair ((void *)&sv.pr.globals, key, com_token))
			Host_Error ("ED_ParseGlobals: parse error");
	}
}

//============================================================================


/*
=============
ED_NewString
=============
*/
char *ED_NewString (char *string)
{
	char	*new, *new_p;
	int		i,l;
	
	l = strlen(string) + 1;
	new = Hunk_Alloc (l);
	new_p = new;

	for (i=0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}
	
	return new;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
bool	ED_ParseEpair (void *base, ddef_t *key, char *s)
{
	int		i;
	char	string[128];
	ddef_t	*def;
	char	*v, *w;
	void	*d;
	dfunction_t	*func;
	
	d = (void *)((int32_t *)base + key->ofs);
	
	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t *)d = PR_SetString(&sv.pr, ED_NewString (s));
		break;
		
	case ev_float:
		*(float *)d = atof (s);
		break;
		
	case ev_vector:
		strcpy (string, s);
		v = string;
		w = string;
		for (i=0 ; i<3 ; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float *)d)[i] = atof (w);
			w = v = v+1;
		}
		break;
		
	case ev_entity:
		*(int32_t *)d = EDICT_TO_PROG(EDICT_NUM(atoi (s)));
		break;
		
	case ev_field:
		def = PR_FindField (&sv.pr, s);
		if (!def)
		{
			Con_Printf ("Can't find field %s\n", s);
			return false;
		}
		*(int32_t *)d = *(int32_t *)(sv.pr.globals + def->ofs);
		break;
	
	case ev_function:
		func = PR_FindFunction (&sv.pr, s);
		if (!func)
		{
			Con_Printf ("Can't find function %s\n", s);
			return false;
		}
		*(func_t *)d = func - sv.pr.functions;
		break;
		
	default:
		break;
	}
	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
char *ED_ParseEdict (char *data, edict_t *ent)
{
	ddef_t		*key;
	bool	anglehack;
	bool	init;
	char		keyname[256];
	int			n;

	init = false;

// clear it
	if (ent != sv.edicts)	// hack
	{
		ED_ClearEdict (ent);
	}

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		data = COM_Parse (data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");
		
// anglehack is to allow QuakeEd to write single scalar angles
// and allow them to be turned into vectors. (FIXME...)
if (!strcmp(com_token, "angle"))
{
	strcpy (com_token, "angles");
	anglehack = true;
}
else
	anglehack = false;

// FIXME: change light to _light to get rid of this hack
if (!strcmp(com_token, "light"))
	strcpy (com_token, "light_lev");	// hack for single light def

		strcpy (keyname, com_token);

		// another hack to fix heynames with trailing spaces
		n = strlen(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value	
		data = COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		init = true;	

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;
		
		key = PR_FindField (&sv.pr, keyname);
		if (!key)
		{
			continue;
		}

if (anglehack)
{
char	temp[32];
strcpy (temp, com_token);
sprintf (com_token, "0 %s 0", temp);
}

		if (!ED_ParseEpair ((void *)(ent + 1), key, com_token))
			Host_Error ("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = true;

	return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile (char *data)
{	
	edict_t		*ent;
	int			inhibit;
	dfunction_t	*func;
	
	ent = NULL;
	inhibit = 0;
	sv_pr_float(time) = sv.time;
	
// parse ents
	while (1)
	{
// parse the opening brace	
		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {",com_token);

		if (!ent)
			ent = EDICT_NUM(0);
		else
			ent = ED_Alloc ();
		data = ED_ParseEdict (data, ent);

// remove things from different skill levels or deathmatch
		if (deathmatch.value)
		{
			if (((int)ed_float(ent, spawnflags) & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
		else if ((current_skill == 0 && ((int)ed_float(ent, spawnflags) & SPAWNFLAG_NOT_EASY))
				|| (current_skill == 1 && ((int)ed_float(ent, spawnflags) & SPAWNFLAG_NOT_MEDIUM))
				|| (current_skill >= 2 && ((int)ed_float(ent, spawnflags) & SPAWNFLAG_NOT_HARD)) )
		{
			ED_Free (ent);	
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ed_int(ent, classname))
		{
			Con_Printf ("No classname for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

	// look for the spawn function
		func = PR_FindFunction (&sv.pr, ed_get_string(ent, classname));

		if (!func)
		{
			Con_Printf(
				"No spawn function for EDICT %i: \"%s\"\n",
				NUM_FOR_EDICT(ent),
				ed_get_string(ent, classname)
			);
			ED_Free (ent);
			continue;
		}

		sv_pr_int(self) = EDICT_TO_PROG(ent);
		PR_ExecuteProgram (&sv.pr, func - sv.pr.functions);
	}	

	Con_DPrintf ("%i entities inhibited\n", inhibit);
}

void ED_Init()
{
	Cmd_AddCommand ("edict", ED_PrintEdict_f);
	Cmd_AddCommand ("edicts", ED_PrintEdicts);
	Cmd_AddCommand ("edictcount", ED_Count);
	Cvar_RegisterVariable (&nomonsters);
	Cvar_RegisterVariable (&gamecfg);
	Cvar_RegisterVariable (&scratch1);
	Cvar_RegisterVariable (&scratch2);
	Cvar_RegisterVariable (&scratch3);
	Cvar_RegisterVariable (&scratch4);
	Cvar_RegisterVariable (&savedgamecfg);
	Cvar_RegisterVariable (&saved1);
	Cvar_RegisterVariable (&saved2);
	Cvar_RegisterVariable (&saved3);
	Cvar_RegisterVariable (&saved4);
}

edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Sys_Error ("EDICT_NUM: bad number %i", n);
	return (edict_t *)((byte *)sv.edicts+ (n)*sv.pr.edict_size);
}

int NUM_FOR_EDICT(edict_t *e)
{
	int		b;
	
	b = (byte *)e - (byte *)sv.edicts;
	b = b / sv.pr.edict_size;
	
	if (b < 0 || b >= sv.num_edicts)
		Sys_Error ("NUM_FOR_EDICT: bad pointer");
	return b;
}
