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

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

bool host_initialized; // true if into command execution

double host_frametime;
double host_time;
double realtime;	// without any filtering or bounding
double oldrealtime; // last frame run
int host_framecount;

int host_hunklevel;

client_t *host_client; // current client

jmp_buf host_abortserver;

cvar_t rcon_password = {"rcon_password", ""};
cvar_t rcon_address = {"rcon_address", ""};

cvar_t password = {"password", "", CVAR_CLIENT_INFO};
cvar_t spectator_password = {"spectator", "", CVAR_CLIENT_INFO};

cvar_t host_framerate = {"host_framerate", "0"};
cvar_t host_timescale = {"host_timescale", "0"}; // set for slow motion
cvar_t host_speeds = {"host_speeds", "0"};		 // set for running times

cvar_t sys_ticrate = {"sys_ticrate", "0.05"};
cvar_t serverprofile = {"serverprofile", "0"};

cvar_t fraglimit = {"fraglimit", "0", CVAR_NOTIFY};
cvar_t timelimit = {"timelimit", "0", CVAR_NOTIFY};
cvar_t teamplay = {"teamplay", "0", CVAR_NOTIFY};

cvar_t samelevel = {"samelevel", "0"};
cvar_t noexit = {"noexit", "0", CVAR_NOTIFY};

#ifndef NDEBUG
cvar_t developer = {"developer", "1"};
#else
cvar_t developer = {"developer", "0"};
#endif

cvar_t skill = {"skill", "1"};			 // 0 - 3
cvar_t deathmatch = {"deathmatch", "0"}; // 0, 1, or 2
cvar_t coop = {"coop", "0"};			 // 0 or 1

cvar_t pausable = {"pausable", "1"};

extern cvar_t maxclients;

extern int cl_framecount;

/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	va_list argptr;
	char string[1024];

	va_start (argptr, message);
	vsprintf (string, message, argptr);
	va_end (argptr);
	Con_DPrintf ("Host_EndGame: %s\n", string);

	if (Host_IsLocalGame ())
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n", string); // dedicated servers exit

	if (cls.demonum != -1)
		CL_NextDemo ();
	else
		CL_Disconnect ();

	longjmp (host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list argptr;
	char string[1024];
	static bool inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;

	SCR_EndLoadingPlaque (); // reenable screen updates

	va_start (argptr, error);
	vsprintf (string, error, argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n", string);

	if (Host_IsLocalGame ())
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n", string); // dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

	longjmp (host_abortserver, 1);
}

static void Host_InitLocal (void)
{
	Host_InitCommands ();

	Cvar_RegisterVariable (src_host, &rcon_password);
	Cvar_RegisterVariable (src_client, &rcon_address);

	Cvar_RegisterVariable (src_host, &password);
	Cvar_RegisterVariable (src_host, &spectator_password);

	Cvar_RegisterVariable (src_host, &host_framerate);
	Cvar_RegisterVariable (src_host, &host_timescale);
	Cvar_RegisterVariable (src_host, &host_speeds);

	Cvar_RegisterVariable (src_host, &sys_ticrate);
	Cvar_RegisterVariable (src_server, &serverprofile);

	Cvar_RegisterVariable (src_server, &fraglimit);
	Cvar_RegisterVariable (src_server, &timelimit);
	Cvar_RegisterVariable (src_server, &teamplay);
	Cvar_RegisterVariable (src_server, &samelevel);
	Cvar_RegisterVariable (src_server, &noexit);
	Cvar_RegisterVariable (src_server, &skill);
	Cvar_RegisterVariable (src_server, &developer);
	Cvar_RegisterVariable (src_server, &deathmatch);
	Cvar_RegisterVariable (src_server, &coop);

	Cvar_RegisterVariable (src_server, &pausable);

	int i = COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		Cvar_SetValue (src_server, "maxclients", atoi (com_argv[i + 1]));
	}
	else
	{
		cls.state = ca_disconnected;
	}

	host_time = 1.0; // so a think at time 0 won't get called
}

/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (void)
{
	FILE *f;

	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (host_initialized & cls.state != ca_dedicated)
	{
		f = fopen (va ("%s/config.cfg", com_gamedir), "w");
		if (!f)
		{
			Con_Printf ("Couldn't write config.cfg.\n");
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		fclose (f);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer (bool crash)
{
	int i;
	int count;
	sizebuf_t buf;
	char message[4];
	double start;

	if (!Host_IsLocalGame ())
		return;

	sv.active = false;
	sv.state = ss_dead;

	Master_Shutdown ();

	// stop all client sounds immediately
	if (cls.state != ca_disconnected)
		CL_Disconnect ();

	SZ_Clear (&net_message[SERVER]);
	MSG_WriteByte (&net_message[SERVER], svc_disconnect);

	for (i = 0, host_client = svs.clients; i < MAX_CLIENTS; i++, host_client++)
		if (host_client->state >= cs_spawned)
			Netchan_Transmit (&host_client->netchan, net_message[SERVER].cursize, net_message[SERVER].data);

	NET_Close (SERVER);

	if (sv_fraglogfile)
	{
		fclose (sv_fraglogfile);
		sv_fraglogfile = NULL;
	}

	PR_ClearStrings (&sv.pr);

	//
	// clear structures
	//
	memset (&sv, 0, sizeof (sv));
	memset (svs.clients, 0, MAX_CLIENTS * sizeof (client_t));
}

/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (void)
{
	Con_DPrintf ("Clearing memory\n");
	CMod_ClearAll ();
	Mod_ClearAll ();
	S_ClearAll ();
	PR_ClearStrings (&sv.pr);
	if (host_hunklevel)
		Hunk_FreeToLowMark (host_hunklevel);

	memset (&sv, 0, sizeof (sv));
	memset (&cl, 0, sizeof (cl));
}

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
static void Host_FilterTime (double time)
{
	realtime += time;

	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	if (host_framerate.value > 0)
		host_frametime = host_framerate.value;
	else
	{ // don't allow really long or short frames
		if (host_frametime > 0.1)
			host_frametime = 0.1;
		if (host_frametime < 0.001)
			host_frametime = 0.001;
	}

	if (host_timescale.value > 0.01)
		host_frametime *= host_timescale.value;
}

/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
static void Host_GetConsoleCommands (void)
{
	char *cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (src_server, cmd);
	}
}

static void Host_ServerFrame (double time)
{
	static double start, end;

	start = Sys_FloatTime ();
	svs.stats.idle += start - end;

	// keep the random time dependent
	rand ();

	// decide the simulation time
	if (!Host_IsPaused ())
		sv.newtime += time;

	// check timeouts
	SV_CheckTimeouts ();

	// toggle the log buffer if full
	SV_CheckLog ();

	// move autonomous things around if enough time has passed
	if (!Host_IsPaused ())
		SV_Physics ();

	// get packets
	SV_ReadPackets ();

	// process console commands
	Cbuf_Execute (src_server);

	SV_CheckVars ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void Host_Frame (double time)
{
	static double time1 = 0;
	static double time2 = 0;
	static double time3 = 0;
	int pass1, pass2, pass3;

	if (setjmp (host_abortserver))
		return; // something bad happened, or the server disconnected

	// keep the random time dependent
	rand ();

	// decide the simulation time
	Host_FilterTime (time);

	// get new key events
	Sys_SendKeyEvents ();

	// allow mice or other external controllers to add commands
	IN_Commands ();

	// process console commands
	Cbuf_Execute (src_client);

	// fetch results from server
	CL_ReadPackets ();

	// if running the server locally, make intentions now
	if (cls.state == ca_disconnected)
	{
	}
	else if (Host_IsLocalGame ())
	{
		CL_SendCmd ();
	}

	//-------------------
	//
	// server operations
	//
	//-------------------

	// check for commands typed to the host
	Host_GetConsoleCommands ();

	if (Host_IsLocalGame ())
		Host_ServerFrame (host_frametime);
	else
		Cbuf_Execute (src_server);

	//-------------------
	//
	// client operations
	//
	//-------------------

	// if running the server remotely, send intentions now after
	// the incoming messages have been read
	if (cls.state == ca_disconnected)
	{
		// resend a connection request if necessary
		CL_CheckForResend ();
	}
	else if (!Host_IsLocalGame ())
	{
		CL_SendCmd ();
	}

	host_time += host_frametime;

	if (cls.state >= ca_connected)
	{
		if (!Host_IsPaused ())
		{
			// Set up prediction for other players
			CL_SetUpPlayerPrediction (false);

			// do client side motion prediction
			CL_PredictMove ();

			// Set up prediction for other players
			CL_SetUpPlayerPrediction (true);
		}

		// build a refresh entity list
		CL_EmitEntities ();
	}

	// update video
	if (host_speeds.value)
		time1 = Sys_FloatTime ();

	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_FloatTime ();

	// update audio
	if (cls.state == ca_active)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, (vec3_t){1, 0, 0}, (vec3_t){0, -1, 0}, (vec3_t){0, 0, 1});

	Music_Update ();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3) * 1000;
		time3 = Sys_FloatTime ();
		pass2 = (time2 - time1) * 1000;
		pass3 = (time3 - time2) * 1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n", pass1 + pass2 + pass3, pass1, pass2, pass3);
	}

	host_framecount++;
	cl_framecount++;
}

extern cvar_t qport;

static void Host_InitNet (void)
{
	int serverPort = PORT_SERVER;
	int p = COM_CheckParm ("-port");
	if (p && p < com_argc)
	{
		serverPort = atoi (com_argv[p + 1]);
		Con_Printf ("Server Port: %i\n", serverPort);
	}

	NET_Init ();
	Netchan_Init ();
	NET_Open (CLIENT, qport.value);
}

void Host_Init (quakeparms_t *parms)
{
	host_parms = *parms;

	size_t minimum_memory = MINIMUM_MEMORY;
	if (quake_mode == QUAKE_LEVELPAK)
		minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = minimum_memory;
	else if (parms->memsize < minimum_memory)
		Sys_Error ("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

	com_argc = parms->argc;
	com_argv = parms->argv;

	Memory_Init (parms->membase, parms->memsize);
	Cbuf_Init ();
	Cmd_Init ();
	V_Init ();
	Chase_Init ();
	COM_Init (parms->basedir);
	Host_InitLocal ();
	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	Con_Init ();
	M_Init ();
	PR_Init ();
	ED_Init ();
	CMod_Init ();
	Host_InitNet ();
	SV_Init ();

	Con_Printf ("Build: " __TIME__ " " __DATE__ "\n");
	Con_Printf ("%4.1f megabyte heap\n", (float)parms->memsize / (1024 * 1024));

	R_InitTextures (); // needed even for dedicated servers

	if (cls.state != ca_dedicated)
	{
		IN_Init ();
		VID_Init ("gfx/palette.lmp");
		Draw_Init ();
		SCR_Init ();
		R_Init ();
		S_Init ();
		Music_Init ();
		Sbar_Init ();
		CL_Init ();
		S_InitBase ();
	}

	Cbuf_InsertText (src_client, "exec quake.rc\n");

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	host_initialized = true;

	Sys_Printf ("========Quake Initialized=========\n");
}

void Host_InitServer (void)
{
	int port = PORT_SERVER;
	int p = COM_CheckParm ("-port");
	if (p && p < com_argc)
		port = atoi (com_argv[p + 1]);

	NET_Open (SERVER, port);
}

/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown (void)
{
	static bool isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

	// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration ();

	Music_Shutdown ();
	NET_Shutdown ();
	S_Shutdown ();
	IN_Shutdown ();

	if (cls.state != ca_dedicated)
		VID_Shutdown ();
}

bool Host_IsLocalGame (void)
{
	return sv.active;
}

bool Host_IsLocalClient (int userid)
{
	return Host_IsLocalGame () && cls.state != ca_dedicated && cl.userid == userid;
}

bool Host_IsPaused (void)
{
	return sv.paused || (maxclients.value <= 1 && key_dest != key_game);
}
