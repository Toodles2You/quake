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
#include "clientdef.h"

static cvar_t *cvar_vars[2];
static char *const cvar_null_string = "";

cvar_t *Cvar_FindVar (cmd_source_e src, char *var_name)
{
	cvar_t *var;

	for (var = cvar_vars[src]; var; var = var->next[src])
		if (!strcmp (var_name, var->name))
			return var;

	return NULL;
}

float Cvar_VariableValue (cmd_source_e src, char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (src, var_name);
	if (!var)
		return 0;
	return atof (var->string);
}

char *Cvar_VariableString (cmd_source_e src, char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (src, var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}

static void Cvar_GetBestVariable (cmd_source_e src, char *partial, int len, char **best, int *bestlen)
{
	cvar_t *cvar;
	int curlen;

	for (cvar = cvar_vars[src]; cvar; cvar = cvar->next[src])
	{
		curlen = abs (strlen (cvar->name) - len);

		if (curlen < *bestlen && !strncmp (partial, cvar->name, len))
		{
			*best = cvar->name;
			*bestlen = curlen;
		}
	}
}

char *Cvar_CompleteVariable (cmd_source_e src, char *partial)
{
	int len;
	char *best = NULL;
	int bestlen = 999;

	len = strlen (partial);

	if (!len)
		return NULL;

	// check partial match
	Cvar_GetBestVariable (src, partial, len, &best, &bestlen);

	// local game queries server commands, too
	if (src == src_client && Host_IsLocalGame ())
		Cvar_GetBestVariable (src_server, partial, len, &best, &bestlen);

	return best;
}

void SV_SendServerInfoChange (char *key, char *value);

void Cvar_Set (cmd_source_e src, char *var_name, char *value)
{
	cvar_t *var;
	bool changed;

	var = Cvar_FindVar (src, var_name);
	if (!var)
	{ // there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return;
	}

	changed = strcmp (var->string, value);

	if ((var->flags & CVAR_SERVER_INFO) && Host_IsLocalGame ())
	{
		Info_SetValueForKey (svs.info, var_name, value, MAX_SERVERINFO_STRING, sv_highchars.value);
		SV_SendServerInfoChange (var_name, value);
	}

	if (var->flags & CVAR_CLIENT_INFO)
	{
		Info_SetValueForKey (cls.userinfo, var_name, value, MAX_INFO_STRING, true);
		if (cls.state >= ca_connected)
		{
			MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
			SZ_Print (&cls.netchan.message, va ("setinfo \"%s\" \"%s\"\n", var_name, value));
		}
	}

	Z_Free (var->string); // free the old value string

	var->string = Z_Malloc (strlen (value) + 1);
	strcpy (var->string, value);
	var->value = atof (var->string);

	if ((var->flags & CVAR_NOTIFY) && changed && Host_IsLocalGame ())
		SV_BroadcastPrintf (PRINT_HIGH, "\"%s\" changed to \"%s\"\n", var->name, var->string);
}

void Cvar_SetValue (cmd_source_e src, char *var_name, float value)
{
	char val[32];

	sprintf (val, "%f", value);
	Cvar_Set (src, var_name, val);
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cmd_source_e src, cvar_t *variable)
{
	char value[512];

	if (src == src_host)
	{
		Cvar_RegisterVariable (src_client, variable);
		Cvar_RegisterVariable (src_server, variable);
		return;
	}

	// first check to see if it has allready been defined
	if (Cvar_FindVar (src, variable->name))
	{
		Con_Printf ("Can't register variable %s, allready defined\n", variable->name);
		return;
	}

	// check for overlap with a command
	if (Cmd_Exists (src, variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}

	// link the variable in
	variable->next[src] = cvar_vars[src];
	cvar_vars[src] = variable;

	// copy the value off, because future sets will Z_Free it
	strcpy (value, variable->string);
	variable->string = Z_Malloc (1);

	// set it through the function to be consistant
	Cvar_Set (src, variable->name, value);
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool Cvar_Command (cmd_source_e src)
{
	cvar_t *v;

	// check variables
	v = Cvar_FindVar (src, Cmd_Argv (0));
	if (!v)
		return false;

	// perform a variable print or set
	if (Cmd_Argc () == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (src, v->name, Cmd_Argv (1));
	return true;
}

/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t *var;

	for (var = cvar_vars[src_client]; var; var = var->next[src_client])
		if (var->flags & CVAR_ARCHIVE)
			fprintf (f, "%s \"%s\"\n", var->name, var->string);
}
