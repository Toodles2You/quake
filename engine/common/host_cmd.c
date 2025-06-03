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

extern cvar_t	maxclients;
extern cvar_t	pausable;
extern cvar_t sv_ticrate;

int	current_skill;

void Mod_Print ();

/*
==================
Host_Quit_f
==================
*/
void Host_Quit_f ()
{
	CL_Disconnect ();
	Host_ShutdownServer(false);
	Sys_Quit ();
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/

static bool Host_CheckLevel (char* server)
{
	char expanded[MAX_QPATH];
	FILE *f;

	// check to make sure the level exists
	sprintf(expanded, "maps/%s.bsp", server);

	COM_FOpenFile(expanded, &f);

	if (!f)
	{
		Con_Printf("Can't find %s\n", expanded);
		return false;
	}

	fclose(f);

	return true;
}

static void Host_ChangingLevel ()
{
	if (!Host_IsLocalGame ())
	{
		return;
	}

	if (cls.download)  // don't change when downloading
	{
		return;
	}

	SV_BroadcastCommand ("changing\n");
	SV_SendMessagesToAll ();

	if (cls.state != ca_dedicated)
	{
		S_StopAllSounds (true);
		cl.intermission = 0;
		cls.state = ca_connected;	// not active anymore, but not disconnected
		Con_Printf ("\nChanging map...\n");
	}
}


/*
======================
Host_Map_f

handle a 
map <servername>
command from the console.  Active clients are kicked off.

Toodles TODO: Warn server hosts that this kicks all clients.
======================
*/
void Host_Map_f ()
{
	char	level[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("map <levelname> : continue game on a new level\n");
		return;
	}
	strcpy (level, Cmd_Argv(1));

	if (!Host_CheckLevel (level))
	{
		return;
	}

	Host_ShutdownServer (false);
	Host_InitServer ();

	svs.serverflags = 0;

	SV_SpawnServer (level, NULL);

	if (Host_IsLocalGame ())
	{
		if (cls.state != ca_dedicated)
		{
			Cmd_ExecuteString (src_client, "connect localhost");
		}
	}
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f ()
{
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char	*startspot = NULL;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("changelevel <levelname> : continue game on a new level\n");
		return;
	}
	if (!Host_IsLocalGame () || cls.demoplayback)
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}

	strcpy (level, Cmd_Argv(1));

	if (!Host_CheckLevel (level))
	{
		return;
	}

	Host_ChangingLevel ();

	SV_SaveSpawnparms ();

	SV_SpawnServer (level, startspot);

	SV_BroadcastCommand ("reconnect\n");
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f ()
{
	char	mapname[MAX_QPATH];
	char	startspot[MAX_QPATH];
	int		i;

	if (cls.demoplayback || !Host_IsLocalGame ())
		return;

	strcpy (mapname, sv.name);	// must copy out, because it gets cleared
								// in sv_spawnserver
	strcpy(startspot, sv.startspot);

	if (!Host_CheckLevel (mapname))
	{
		return;
	}

	Host_ChangingLevel ();

	for (i = 0, host_client = svs.clients; i < MAX_CLIENTS; i++, host_client++)
	{
		if (host_client->state != cs_spawned)
			continue;

		// needs to reconnect
		host_client->state = cs_connected;
	}

	SV_SpawnServer (mapname, startspot);

	SV_BroadcastCommand ("reconnect\n");
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define	SAVEGAME_VERSION	5

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current 
===============
*/
void Host_SavegameComment (char *text)
{
	int		i;
	char	kills[20];

	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		text[i] = ' ';
	memcpy (text, cl.levelname, strlen(cl.levelname));
	sprintf (kills,"kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	memcpy (text+22, kills, strlen(kills));
// convert space to _ to make stdio happy
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		if (text[i] == ' ')
			text[i] = '_';
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f ()
{
	char	name[256];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];

	if (!Host_IsLocalGame ())
	{
		Con_Printf ("Not playing a local game.\n");
		return;
	}

	if (cl.intermission)
	{
		Con_Printf ("Can't save in intermission.\n");
		return;
	}

	if (maxclients.value > 1)
	{
		Con_Printf ("Can't save multiplayer games.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}
		
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (svs.clients[i].state == cs_spawned
		 && (ed_float(svs.clients[i].edict, health) <= 0) )
		{
			Con_Printf ("Can't savegame with a dead player\n");
			return;
		}
	}

	sprintf (name, "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension (name, ".sav");
	
	Con_Printf ("Saving game to %s...\n", name);
	f = fopen (name, "w");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}
	
	fprintf (f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment (comment);
	fprintf (f, "%s\n", comment);
	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
	fprintf (f, "%d\n", current_skill);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n",sv.time);

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv.lightstyles[i])
			fprintf (f, "%s\n", sv.lightstyles[i]);
		else
			fprintf (f,"m\n");
	}


	ED_WriteGlobals (f);
	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ED_Write (f, EDICT_NUM(i));
		fflush (f);
	}
	fclose (f);
	Con_Printf ("done.\n");
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f ()
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, tfloat;
	char	str[32768], *start;
	int		i, r, j;
	edict_t	*ent;
	int		entnum;
	int		version;
	float			spawn_parms[NUM_SPAWN_PARMS];

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("load <savename> : load a game\n");
		return;
	}

	cls.demonum = -1;		// stop demo loop in case this fails

	sprintf (name, "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension (name, ".sav");
	
// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

	Con_Printf ("Loading game from %s...\n", name);
	f = fopen (name, "r");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}

	fscanf (f, "%i\n", &version);
	if (version != SAVEGAME_VERSION)
	{
		fclose (f);
		Con_Printf ("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return;
	}

	Host_ShutdownServer (false);

	fscanf (f, "%s\n", str);
	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		fscanf (f, "%f\n", &spawn_parms[i]);
// this silliness is so we can load 1.06 save files, which have float skill values
	fscanf (f, "%f\n", &tfloat);
	current_skill = (int)(tfloat + 0.1);
	Cvar_SetValue (src_server, "skill", (float)current_skill);

	Cvar_SetValue (src_server, "deathmatch", 0);
	Cvar_SetValue (src_server, "coop", 0);
	Cvar_SetValue (src_server, "teamplay", 0);

	fscanf (f, "%s\n",mapname);
	fscanf (f, "%f\n",&time);

	Host_InitServer ();

	SV_SpawnServer (mapname, NULL);

	if (!Host_IsLocalGame ())
	{
		Con_Printf ("Couldn't load map\n");
		return;
	}
	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

// load the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		sv.lightstyles[i] = Hunk_Alloc (strlen(str)+1);
		strcpy (sv.lightstyles[i], str);
	}

// load the edicts out of the savegame file
	entnum = -1;		// -1 is the globals
	while (!feof(f))
	{
		for (i=0 ; i<sizeof(str)-1 ; i++)
		{
			r = fgetc (f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str)-1)
			Sys_Error ("Loadgame buffer overflow");
		str[i] = 0;
		start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break;		// end of file
		if (strcmp(com_token,"{"))
			Sys_Error ("First token isn't a brace");
			
		if (entnum == -1)
		{	// parse the global vars
			ED_ParseGlobals (start);
		}
		else
		{	// parse an edict

			ent = EDICT_NUM(entnum);
			ED_ClearEdict (ent);
			ED_ParseEdict (start, ent);
	
		// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict (ent, false);
		}

		entnum++;
	}
	
	sv.num_edicts = entnum;
	sv.newtime = sv.oldtime = time;
	sv.deltatime = sv_ticrate.value;

	fclose (f);

	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		svs.clients->spawn_parms[i] = spawn_parms[i];

	if (cls.state != ca_dedicated)
	{
		Cmd_ExecuteString (src_client, "connect localhost");
	}
}

void SaveGamestate()
{
	char	name[256];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];
	edict_t	*ent;

	sprintf (name, "%s/%s.gip", com_gamedir, sv.name);
	
	Con_Printf ("Saving game to %s...\n", name);
	f = fopen (name, "w");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}
	
	fprintf (f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment (comment);
	fprintf (f, "%s\n", comment);
//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
	fprintf (f, "%f\n", skill.value);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n", sv.time);

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv.lightstyles[i])
			fprintf (f, "%s\n", sv.lightstyles[i]);
		else
			fprintf (f,"m\n");
	}


	for (i=MAX_CLIENTS+1 ; i<sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(i);
		if ((int)ed_float(ent, flags) & FL_ARCHIVE_OVERRIDE)
			continue;
		fprintf (f, "%i\n",i);
		ED_Write (f, ent);
		fflush (f);
	}
	fclose (f);
	Con_Printf ("done.\n");
}

int LoadGamestate(char *level, char *startspot)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, sk;
	char	str[32768], *start;
	int		i, r;
	edict_t	*ent;
	int		entnum;
	int		version;
//	float	spawn_parms[NUM_SPAWN_PARMS];

	sprintf (name, "%s/%s.gip", com_gamedir, level);
	
	Con_Printf ("Loading game from %s...\n", name);
	f = fopen (name, "r");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return -1;
	}

	fscanf (f, "%i\n", &version);
	if (version != SAVEGAME_VERSION)
	{
		fclose (f);
		Con_Printf ("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return -1;
	}
	fscanf (f, "%s\n", str);
//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		fscanf (f, "%f\n", &spawn_parms[i]);
	fscanf (f, "%f\n", &sk);
	Cvar_SetValue (src_server, "skill", sk);

	fscanf (f, "%s\n",mapname);
	fscanf (f, "%f\n",&time);

	SV_SpawnServer (mapname, startspot);

	if (!Host_IsLocalGame ())
	{
		Con_Printf ("Couldn't load map\n");
		return -1;
	}

// load the light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		sv.lightstyles[i] = Hunk_Alloc (strlen(str)+1);
		strcpy (sv.lightstyles[i], str);
	}

// load the edicts out of the savegame file
	while (!feof(f))
	{
		fscanf (f, "%i\n",&entnum);
		for (i=0 ; i<sizeof(str)-1 ; i++)
		{
			r = fgetc (f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str)-1)
			Sys_Error ("Loadgame buffer overflow");
		str[i] = 0;
		start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break;		// end of file
		if (strcmp(com_token,"{"))
			Sys_Error ("First token isn't a brace");
			
		// parse an edict

		ent = EDICT_NUM(entnum);
		ED_ClearEdict (ent);
		ED_ParseEdict (start, ent);
	
		// link it into the bsp tree
		if (!ent->free)
			SV_LinkEdict (ent, false);
	}
	
//	sv.num_edicts = entnum;
	sv.newtime = sv.oldtime = time;
	sv.deltatime = sv_ticrate.value;
	fclose (f);

//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		svs.clients->spawn_parms[i] = spawn_parms[i];

	return 0;
}

// changing levels within a unit
void Host_Changelevel2_f ()
{
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char	*startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}
	if (!Host_IsLocalGame () || cls.demoplayback)
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}

	strcpy (level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
		startspot = NULL;
	else
	{
		strcpy (_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms ();

	// save the current level's state
	SaveGamestate ();

	// try to restore the new level
	if (LoadGamestate (level, startspot))
		SV_SpawnServer (level, startspot);
}


//============================================================================

	
void Host_Version_f ()
{
	Con_Printf ("Version "QUAKE_VERSION"\n");
	Con_Printf ("Build: "__TIME__" "__DATE__"\n");
}

//===========================================================================

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f ()
{
	int		i, c;

	if (cls.state == ca_dedicated)
	{
		if (!Host_IsLocalGame ())
			Cbuf_AddText (src_server, "map start\n");
		return;
	}

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf ("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_Printf ("%i demo(s) in loop\n", c);

	for (i=1 ; i<c+1 ; i++)
		strncpy (cls.demos[i-1], Cmd_Argv(i), sizeof(cls.demos[0])-1);

	if (!Host_IsLocalGame () && cls.demonum != -1 && !cls.demoplayback)
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f ()
{
	if (cls.state == ca_dedicated)
		return;
	if (cls.demonum == -1)
		cls.demonum = 1;
	CL_Disconnect_f ();
	CL_NextDemo ();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f ()
{
	if (cls.state == ca_dedicated)
		return;
	if (!cls.demoplayback)
		return;
	CL_StopPlayback ();
	CL_Disconnect ();
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands ()
{
	Cmd_AddCommand (src_host, "quit", Host_Quit_f);
	Cmd_AddCommand (src_host, "map", Host_Map_f);
	Cmd_AddCommand (src_server, "restart", Host_Restart_f);
	Cmd_AddCommand (src_server, "changelevel", Host_Changelevel_f);
	Cmd_AddCommand (src_server, "changelevel2", Host_Changelevel2_f);
	Cmd_AddCommand (src_host, "version", Host_Version_f);
	Cmd_AddCommand (src_host, "load", Host_Loadgame_f);
	Cmd_AddCommand (src_server, "save", Host_Savegame_f);

	Cmd_AddCommand (src_client, "startdemos", Host_Startdemos_f);
	Cmd_AddCommand (src_client, "demos", Host_Demos_f);
	Cmd_AddCommand (src_client, "stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand (src_client, "mcache", Mod_Print);
}
