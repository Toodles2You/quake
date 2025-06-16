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
#include "serverdef.h"

cmd_source_e cmd_source;

#define MAX_ALIAS_NAME 32

typedef struct cmdalias_s
{
	struct cmdalias_s *next;
	char name[MAX_ALIAS_NAME];
	char *value;
} cmdalias_t;

static cmdalias_t *cmd_alias;

static bool cmd_wait[2];

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void Cmd_Wait_f (void)
{
	cmd_wait[cmd_source] = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

static sizebuf_t cmd_text[2];
static byte cmd_text_buf[2][8192];

void Cbuf_Init (void)
{
	cmd_text[src_client].data = cmd_text_buf[src_client];
	cmd_text[src_client].maxsize = sizeof (cmd_text_buf[src_client]);

	cmd_text[src_server].data = cmd_text_buf[src_server];
	cmd_text[src_server].maxsize = sizeof (cmd_text_buf[src_server]);
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (cmd_source_e src, char *text)
{
	int l;

	l = strlen (text);

	if (cmd_text[src].cursize + l >= cmd_text[src].maxsize)
	{
		Con_Printf ("Cbuf_AddText: overflow\n");
		return;
	}

	SZ_Write (&cmd_text[src], text, strlen (text));
}

/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (cmd_source_e src, char *text)
{
	char *temp;
	int templen;

	// copy off any commands still remaining in the exec buffer
	templen = cmd_text[src].cursize;
	if (templen)
	{
		temp = Z_Malloc (templen);
		memcpy (temp, cmd_text[src].data, templen);
		SZ_Clear (&cmd_text[src]);
	}
	else
		temp = NULL; // shut up compiler

	// add the entire text of the file
	Cbuf_AddText (src, text);
	SZ_Write (&cmd_text[src], "\n", 1);

	// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text[src], temp, templen);
		Z_Free (temp);
	}
}

void Cbuf_Execute (cmd_source_e src)
{
	int i;
	char *text;
	char line[1024];
	int quotes;

	while (cmd_text[src].cursize)
	{
		// find a \n or ; line break
		text = (char *)cmd_text[src].data;

		quotes = 0;
		for (i = 0; i < cmd_text[src].cursize; i++)
		{
			if (text[i] == '"')
				quotes++;
			if (!(quotes & 1) && text[i] == ';')
				break; // don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		memcpy (line, text, i);
		line[i] = 0;

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec, alias) can insert data at the
		// beginning of the text buffer

		if (i == cmd_text[src].cursize)
			cmd_text[src].cursize = 0;
		else
		{
			i++;
			cmd_text[src].cursize -= i;
			memcpy (text, text + i, cmd_text[src].cursize);
		}

		// execute the command line
		Cmd_ExecuteString (src, line);

		if (cmd_wait[src])
		{ // skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait[src] = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
static void Cmd_StuffCmds_f (void)
{
	int i, j;
	int s;
	char *text, *build, c;

	if (Cmd_Argc () != 1)
	{
		Con_Printf ("stuffcmds : execute command line parameters\n");
		return;
	}

	// build the combined string to parse from
	s = 0;
	for (i = 1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue; // NEXTSTEP nulls out -NXHost
		s += strlen (com_argv[i]) + 1;
	}
	if (!s)
		return;

	text = Z_Malloc (s + 1);
	text[0] = 0;
	for (i = 1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue; // NEXTSTEP nulls out -NXHost
		strcat (text, com_argv[i]);
		if (i != com_argc - 1)
			strcat (text, " ");
	}

	// pull out the commands
	build = Z_Malloc (s + 1);
	build[0] = 0;

	for (i = 0; i < s - 1; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j = i; (text[j] != '+') && (text[j] != '-') && (text[j] != 0); j++);

			c = text[j];
			text[j] = 0;

			strcat (build, text + i);
			strcat (build, "\n");
			text[j] = c;
			i = j - 1;
		}
	}

	if (build[0])
		Cbuf_InsertText (cmd_source, build);

	Z_Free (text);
	Z_Free (build);
}

static void Cmd_Exec_f (void)
{
	char *f;
	int mark;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	mark = Hunk_LowMark ();
	f = (char *)COM_LoadHunkFile (Cmd_Argv (1));
	if (!f)
	{
		Con_Printf ("couldn't exec %s\n", Cmd_Argv (1));
		return;
	}
	if (!Cvar_Command (cmd_source) && developer.value)
		Con_Printf ("execing %s\n", Cmd_Argv (1));

	Cbuf_InsertText (cmd_source, f);
	Hunk_FreeToLowMark (mark);
}

/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (void)
{
	int i;

	for (i = 1; i < Cmd_Argc (); i++)
		Con_Printf ("%s ", Cmd_Argv (i));
	Con_Printf ("\n");
}

char *CopyString (char *in)
{
	char *out;

	out = Z_Malloc (strlen (in) + 1);
	strcpy (out, in);
	return out;
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
static void Cmd_Alias_f (void)
{
	cmdalias_t *a;
	char cmd[1024];
	int i, c;
	char *s;

	if (Cmd_Argc () == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias; a; a = a->next)
			Con_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv (1);
	if (strlen (s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias allready exists, reuse it
	for (a = cmd_alias; a; a = a->next)
	{
		if (!strcmp (s, a->name))
		{
			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		a = Z_Malloc (sizeof (cmdalias_t));
		a->next = cmd_alias;
		cmd_alias = a;
	}
	strcpy (a->name, s);

	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string
	c = Cmd_Argc ();
	for (i = 2; i < c; i++)
	{
		strcat (cmd, Cmd_Argv (i));
		if (i != c)
			strcat (cmd, " ");
	}
	strcat (cmd, "\n");

	a->value = CopyString (cmd);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s *next[2];
	char *name;
	xcommand_t function;
} cmd_function_t;

#define MAX_ARGS 80

static int cmd_argc[2];
static char *cmd_argv[2][MAX_ARGS];
static char *cmd_null_string = "";
static char *cmd_args[2] = {NULL, NULL};

static cmd_function_t *cmd_functions[2]; // possible commands to execute

int Cmd_Argc (void)
{
	return cmd_argc[cmd_source];
}

char *Cmd_Argv (int arg)
{
	if (arg >= cmd_argc[cmd_source])
		return cmd_null_string;
	return cmd_argv[cmd_source][arg];
}

char *Cmd_Args (void)
{
	if (!cmd_args[cmd_source])
		return "";
	return cmd_args[cmd_source];
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString (cmd_source_e src, char *text)
{
	int i;

	// clear the args from the last string
	for (i = 0; i < cmd_argc[src]; i++)
		Z_Free (cmd_argv[src][i]);

	cmd_source = src;
	cmd_argc[src] = 0;
	cmd_args[src] = NULL;

	while (1)
	{
		// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
			text++;

		if (*text == '\n')
		{ // a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		if (cmd_argc[src] == 1)
			cmd_args[src] = text;

		text = COM_Parse (text);
		if (!text)
			return;

		if (cmd_argc[src] < MAX_ARGS)
		{
			cmd_argv[src][cmd_argc[src]] = Z_Malloc (strlen (com_token) + 1);
			strcpy (cmd_argv[src][cmd_argc[src]], com_token);
			cmd_argc[src]++;
		}
	}
}

void Cmd_AddCommand (cmd_source_e src, char *cmd_name, xcommand_t function)
{
	cmd_function_t *cmd;

	if (host_initialized) // because hunk allocation would get stomped
		Sys_Error ("Cmd_AddCommand after host_initialized");

	if (src == src_host)
	{
		Cmd_AddCommand (src_client, cmd_name, function);
		Cmd_AddCommand (src_server, cmd_name, function);
		return;
	}

	// fail if the command is a variable name
	if (Cvar_VariableString (src, cmd_name)[0])
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}

	// fail if the command already exists
	for (cmd = cmd_functions[src]; cmd; cmd = cmd->next[src])
	{
		if (!strcmp (cmd_name, cmd->name))
		{
			Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
		}
	}

	cmd = Hunk_Alloc (sizeof (cmd_function_t));
	cmd->name = cmd_name;
	cmd->function = function;
	cmd->next[src] = cmd_functions[src];
	cmd_functions[src] = cmd;
}

bool Cmd_Exists (cmd_source_e src, char *cmd_name)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions[src]; cmd; cmd = cmd->next[src])
		if (!strcmp (cmd_name, cmd->name))
			return true;

	return false;
}

static void Cmd_GetBestCommand (cmd_source_e src, char *partial, int len, char **best, int *bestlen)
{
	cmd_function_t *cmd;
	int curlen;

	for (cmd = cmd_functions[src]; cmd; cmd = cmd->next[src])
	{
		curlen = abs (strlen (cmd->name) - len);

		if (curlen < *bestlen && !strncmp (partial, cmd->name, len))
		{
			*best = cmd->name;
			*bestlen = curlen;
		}
	}
}

char *Cmd_CompleteCommand (cmd_source_e src, char *partial)
{
	int len;
	char *best = NULL;
	int bestlen = 999;

	len = strlen (partial);

	if (!len)
		return NULL;

	// check functions
	Cmd_GetBestCommand (src, partial, len, &best, &bestlen);

	// local game queries server commands, too
	if (src == src_client && Host_IsLocalGame ())
		Cmd_GetBestCommand (src_server, partial, len, &best, &bestlen);

	return best;
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}

	if (cls.demoplayback)
		return; // not really connected

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, Cmd_Argv (0));
	if (Cmd_Argc () > 1)
	{
		SZ_Print (&cls.netchan.message, " ");
		SZ_Print (&cls.netchan.message, Cmd_Args ());
	}
}

// don't forward the first argument
static void Cmd_ForwardToServer_f (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv (0));
		return;
	}

	if (cls.demoplayback)
		return; // not really connected

	if (Cmd_Argc () > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, Cmd_Args ());
	}
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void Cmd_ExecuteString (cmd_source_e src, char *text)
{
	cmd_function_t *cmd;
	cmdalias_t *a;

	Cmd_TokenizeString (src, text);

	// execute the command line
	if (!Cmd_Argc ())
		return; // no tokens

	// check functions
	for (cmd = cmd_functions[src]; cmd; cmd = cmd->next[src])
	{
		if (!strcasecmp (cmd_argv[src][0], cmd->name))
		{
			cmd->function ();
			return;
		}
	}

	// check alias
	for (a = cmd_alias; a; a = a->next)
	{
		if (!strcasecmp (cmd_argv[src][0], a->name))
		{
			Cbuf_InsertText (src, a->value);
			return;
		}
	}

	// check cvars
	if (Cvar_Command (src))
		return;

	if (src == src_client)
		Cmd_ForwardToServer ();
	else
		Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv (0));
}

void Cmd_Init (void)
{
	//
	// register our commands
	//
	Cmd_AddCommand (src_host, "stuffcmds", Cmd_StuffCmds_f);
	Cmd_AddCommand (src_host, "exec", Cmd_Exec_f);
	Cmd_AddCommand (src_host, "echo", Cmd_Echo_f);
	Cmd_AddCommand (src_host, "alias", Cmd_Alias_f);
	Cmd_AddCommand (src_client, "cmd", Cmd_ForwardToServer_f);
	Cmd_AddCommand (src_host, "wait", Cmd_Wait_f);
}
