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

#include "serverdef.h"

ddef_t *PR_GlobalAtOfs (progs_state_t *pr, int ofs)
{
	ddef_t *def;
	int i;

	for (i = 0; i < pr->progs->numglobaldefs; i++)
	{
		def = &pr->globaldefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

ddef_t *PR_FieldAtOfs (progs_state_t *pr, int ofs)
{
	ddef_t *def;
	int i;

	for (i = 0; i < pr->progs->numfielddefs; i++)
	{
		def = &pr->fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

ddef_t *PR_FindField (progs_state_t *pr, char *name)
{
	ddef_t *def;
	int i;

	for (i = 0; i < pr->progs->numfielddefs; i++)
	{
		def = &pr->fielddefs[i];
		if (!strcmp (PR_GetString (pr, def->s_name), name))
			return def;
	}
	return NULL;
}

ddef_t *PR_FindGlobal (progs_state_t *pr, char *name)
{
	ddef_t *def;
	int i;

	for (i = 0; i < pr->progs->numglobaldefs; i++)
	{
		def = &pr->globaldefs[i];
		if (!strcmp (PR_GetString (pr, def->s_name), name))
			return def;
	}
	return NULL;
}

dfunction_t *PR_FindFunction (progs_state_t *pr, char *name)
{
	dfunction_t *func;
	int i;

	for (i = 0; i < pr->progs->numfunctions; i++)
	{
		func = &pr->functions[i];
		if (!strcmp (PR_GetString (pr, func->s_name), name))
			return func;
	}
	return NULL;
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *PR_ValueString (progs_state_t *pr, etype_t type, eval_t *val)
{
	static char line[256];
	ddef_t *def;
	dfunction_t *f;

	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_GetString (pr, val->string));
		break;
	case ev_entity:
		sprintf (line, "entity %i", ED_ForNum (PROG_TO_EDICT (val->edict)));
		break;
	case ev_function:
		f = pr->functions + val->function;
		sprintf (line, "%s()", PR_GetString (pr, f->s_name));
		break;
	case ev_field:
		def = PR_FieldAtOfs (pr, val->_int);
		sprintf (line, ".%s", PR_GetString (pr, def->s_name));
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%5.1f", val->_float);
		break;
	case ev_vector:
		sprintf (line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf (line, "pointer");
		break;
	default:
		sprintf (line, "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char *PR_UglyValueString (progs_state_t *pr, etype_t type, eval_t *val)
{
	static char line[256];
	ddef_t *def;
	dfunction_t *f;

	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_GetString (pr, val->string));
		break;
	case ev_entity:
		sprintf (line, "%i", ED_ForNum (PROG_TO_EDICT (val->edict)));
		break;
	case ev_function:
		f = pr->functions + val->function;
		sprintf (line, "%s", PR_GetString (pr, f->s_name));
		break;
	case ev_field:
		def = PR_FieldAtOfs (pr, val->_int);
		sprintf (line, "%s", PR_GetString (pr, def->s_name));
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%f", val->_float);
		break;
	case ev_vector:
		sprintf (line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		sprintf (line, "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char *PR_GlobalString (progs_state_t *pr, int ofs)
{
	char *s;
	int i;
	ddef_t *def;
	void *val;
	static char line[128];

	val = (void *)&pr->globals[ofs];
	def = PR_GlobalAtOfs (pr, ofs);
	if (!def)
	{
		sprintf (line, "%i(?)", ofs);
	}
	else
	{
		s = PR_ValueString (pr, def->type, val);
		sprintf (line, "%i(%s)%s", ofs, PR_GetString (pr, def->s_name), s);
	}

	i = strlen (line);
	for (; i < 20; i++)
		strcat (line, " ");
	strcat (line, " ");

	return line;
}

char *PR_GlobalStringNoContents (progs_state_t *pr, int ofs)
{
	int i;
	ddef_t *def;
	static char line[128];

	def = PR_GlobalAtOfs (pr, ofs);
	if (!def)
		sprintf (line, "%i(?)", ofs);
	else
		sprintf (line, "%i(%s)", ofs, PR_GetString (pr, def->s_name));

	i = strlen (line);
	for (; i < 20; i++)
		strcat (line, " ");
	strcat (line, " ");

	return line;
}

int PR_LoadProgs (progs_state_t *pr, char *filename, int version, int crc)
{
	int i;
	char num[32];

	memset (pr, 0, sizeof (progs_state_t));

	CRC_Init (&pr->crc);

	pr->progs = (dprograms_t *)COM_LoadHunkFile (filename);
	if (!pr->progs)
	{
		Con_DPrintf ("PR_LoadProgs: couldn't load progs.dat");
		return 1;
	}
	Con_DPrintf ("Programs occupy %iK.\n", com_filesize / 1024);

	for (i = 0; i < com_filesize; i++)
		CRC_ProcessByte (&pr->crc, ((byte *)pr->progs)[i]);

	// add prog crc to the serverinfo
	sprintf (num, "%i", pr->crc);
	Info_SetValueForStarKey (svs.info, "*progs", num, MAX_SERVERINFO_STRING, sv_highchars.value);

	if (version != PROG_VERSION_ANY && version != pr->progs->version)
	{
		Con_Printf ("progs.dat has wrong version number (%i should be %i)", pr->progs->version, version);
		return 1;
	}
	if (crc != PROG_CRC_ANY && crc != pr->progs->crc)
	{
		Con_Printf ("progs.dat system vars have been modified, progdefs.h is out of date");
		return 1;
	}

	pr->functions = (dfunction_t *)((byte *)pr->progs + pr->progs->ofs_functions);
	pr->strings = (char *)pr->progs + pr->progs->ofs_strings;
	pr->globaldefs = (ddef_t *)((byte *)pr->progs + pr->progs->ofs_globaldefs);
	pr->fielddefs = (ddef_t *)((byte *)pr->progs + pr->progs->ofs_fielddefs);
	pr->statements = (dstatement_t *)((byte *)pr->progs + pr->progs->ofs_statements);
	pr->globals = (float *)((byte *)pr->progs + pr->progs->ofs_globals);

	*(size_t *)&pr->edict_size = sizeof (edict_t) + pr->progs->entityfields * 4;

	for (i = 0; i < pr->progs->numfielddefs; i++)
		if (pr->fielddefs[i].type & DEF_SAVEGLOBAL)
		{
			Con_Printf ("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
			return 1;
		}

	PR_SetEngineString (pr, "");

	return 0;
}

void PR_BuildStructs (progs_state_t *pr, uint32_t *global_struct, pr_field_t *global_fields, uint32_t *field_struct, pr_field_t *fields)
{
	int i;
	ddef_t *def;

	if (global_struct)
	{
		// get the offsets of the global vars
		for (i = 0; global_fields[i].name != NULL; i++)
		{
			def = PR_FindGlobal (pr, global_fields[i].name);
			if (!def)
			{
				global_struct[i] = 0;
				if (!global_fields[i].optional)
				{
					Host_Error ("PR_LoadProgs: progs.dat globaldefs missing %s\n", global_fields[i].name);
					break;
				}
				Con_DPrintf ("PR_LoadProgs: progs.dat globaldefs does not define %s\n", global_fields[i].name);
				continue;
			}
			if (def->type != ev_function)
			{
				global_struct[i] = def->ofs;
			}
			else
			{
				// C code passes these to PR_ExecuteProgram directly
				global_struct[i] = *(int32_t *)(pr->globals + def->ofs);
			}
		}
		pr->global_struct = global_struct;
	}

	if (field_struct)
	{
		// get the offsets of the entity vars
		for (i = 0; fields[i].name != NULL; i++)
		{
			def = PR_FindField (pr, fields[i].name);
			if (!def)
			{
				field_struct[i] = 0;
				if (!fields[i].optional)
				{
					Host_Error ("PR_LoadProgs: progs.dat fielddefs missing %s\n", fields[i].name);
					break;
				}
				Con_DPrintf ("PR_LoadProgs: progs.dat fielddefs does not define %s\n", fields[i].name);
				continue;
			}
			field_struct[i] = def->ofs;
		}
		pr->field_struct = field_struct;
	}
}

void PR_Init (void)
{
	Cmd_AddCommand (src_server, "profile", PR_Profile_f);
}
