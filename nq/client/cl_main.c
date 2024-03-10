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

#include "clientdef.h"

extern cvar_t qport;

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
cvar_t	cl_name = {"_cl_name", "player", CVAR_ARCHIVE};
cvar_t	cl_color = {"_cl_color", "0", CVAR_ARCHIVE};

cvar_t	cl_timeout = {"cl_timeout", "60"};
cvar_t	cl_shownet = {"cl_shownet","0"};	// can be 0, 1, or 2
cvar_t	cl_nolerp = {"cl_nolerp","0"};
cvar_t	cl_sbar = {"cl_sbar", "1", CVAR_ARCHIVE};
cvar_t	cl_maxfps = {"cl_maxfps", "72"};
cvar_t	cl_showfps = {"cl_showfps", "0"};

cvar_t	lookspring = {"lookspring","0", CVAR_ARCHIVE};
cvar_t	lookstrafe = {"lookstrafe","0", CVAR_ARCHIVE};
cvar_t	sensitivity = {"sensitivity","3", CVAR_ARCHIVE};

cvar_t	m_pitch = {"m_pitch","0.022", CVAR_ARCHIVE};
cvar_t	m_yaw = {"m_yaw","0.022", CVAR_ARCHIVE};
cvar_t	m_forward = {"m_forward","1", CVAR_ARCHIVE};
cvar_t	m_side = {"m_side","0.8", CVAR_ARCHIVE};

cvar_t	entlatency = {"entlatency", "20"};
cvar_t	cl_predict_players = {"cl_predict_players", "1"};
cvar_t	cl_predict_players2 = {"cl_predict_players2", "1"};
cvar_t	cl_solid_players = {"cl_solid_players", "1"};


client_static_t	cls;
client_state_t	cl;
// FIXME: put these on hunk?
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_numvisedicts;
entity_t		*cl_visedicts;

static double connect_time = -1; // for connection retransmits

/*
=======================
CL_SendConnectPacket

called by CL_Connect_f and CL_CheckResend
======================
*/
static void CL_SendConnectPacket ()
{
	netadr_t adr;
	char data[2048];
	double t1, t2;
	// JACK: Fixed bug where DNS lookups would cause two connects real fast
	//       Now, adds lookup time to the connect time.
	//		 Should I add it to realtime instead?!?!

	if (cls.state != ca_disconnected)
	{
		return;
	}

	t1 = Sys_FloatTime();

	if (!NET_StringToAdr(cls.servername, &adr))
	{
		Con_Printf("Bad server address\n");
		connect_time = -1;
		return;
	}

	if (!NET_IsClientLegal(&adr))
	{
		Con_Printf("Illegal server address\n");
		connect_time = -1;
		return;
	}

	if (adr.port == 0)
	{
		adr.port = BigShort(PORT_SERVER);
	}

	t2 = Sys_FloatTime();

	connect_time = realtime + t2 - t1; // for retransmit requests

	cls.qport = qport.value;

	Info_SetValueForStarKey(cls.userinfo, "*ip", "FU", MAX_INFO_STRING);

	//	Con_Printf ("Connecting to %s...\n", cls.servername);
	sprintf(data, "%c%c%c%cconnect %i %i %i \"%s\"\n",
			255, 255, 255, 255, PROTOCOL_VERSION, cls.qport, cls.challenge, cls.userinfo);

	NET_SendPacket(CLIENT, strlen(data), data, adr);
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend ()
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;

	if (connect_time == -1)
	{
		return;
	}
	if (cls.state != ca_disconnected)
	{
		return;
	}
	if (connect_time && realtime - connect_time < 5.0)
	{
		return;
	}

	t1 = Sys_FloatTime ();

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_Printf ("Bad server address\n");
		connect_time = -1;
		return;
	}

	if (!NET_IsClientLegal(&adr))
	{
		Con_Printf ("Illegal server address\n");
		connect_time = -1;
		return;
	}

	if (adr.port == 0)
	{
		adr.port = BigShort (PORT_SERVER);
	}

	t2 = Sys_FloatTime ();

	connect_time = realtime+t2-t1;	// for retransmit requests

	Con_Printf ("Connecting to %s...\n", cls.servername);
	sprintf (data, "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);

	NET_SendPacket (CLIENT, strlen(data), data, adr);
}

void CL_BeginServerConnect()
{
	connect_time = 0;
	CL_CheckForResend();
}

/*
================
CL_Connect_f

================
*/
static void CL_Connect_f ()
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: connect <server>\n");
		return;	
	}
	
	server = Cmd_Argv (1);

	CL_Disconnect ();

	strncpy (cls.servername, server, sizeof(cls.servername)-1);
	CL_BeginServerConnect();
}

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState ()
{
	int			i;

	S_StopAllSounds (true);

	if (!Host_IsLocalGame ())
		Host_ClearMemory ();

	CL_ClearTEnts ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	SZ_Clear (&cls.netchan.message);

// clear other arrays	
	memset (cl_efrags, 0, sizeof(cl_efrags));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));

//
// allocate the efrags and chain together into a free list
//
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect ()
{
	byte final[10];

	connect_time = -1;

// stop sounds (especially looping!)
	S_StopAllSounds (true);
	CDAudio_Stop ();
	
// if running a local server, shut it down
	if (cls.demoplayback)
	{
		CL_StopPlayback ();
	}
	else if (cls.state != ca_disconnected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		final[0] = clc_stringcmd;
		strcpy (final+1, "drop");
		Netchan_Transmit (&cls.netchan, 6, final);
		Netchan_Transmit (&cls.netchan, 6, final);
		Netchan_Transmit (&cls.netchan, 6, final);

		cls.state = ca_disconnected;
		if (Host_IsLocalGame ())
			Host_ShutdownServer(false);

		cls.demoplayback = cls.demorecording = cls.timedemo = false;
	}
	// Cam_Reset();

	if (cls.download)
	{
		fclose(cls.download);
		cls.download = NULL;
	}

	CL_StopUpload();

}

void CL_Disconnect_f ()
{
	CL_Disconnect ();
	if (Host_IsLocalGame ())
		Host_ShutdownServer (false);
}


/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo ()
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
static void CL_Changing_f ()
{
	if (cls.download)  // don't change when downloading
	{
		return;
	}

	S_StopAllSounds (true);
	cl.intermission = 0;
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Con_Printf ("\nChanging map...\n");
}

/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f ()
{
	if (cls.download)  // don't change when downloading
	{
		return;
	}

	S_StopAllSounds (true);

	if (cls.state == ca_connected)
	{
		Con_Printf ("reconnecting...\n");
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		return;
	}

	if (!*cls.servername) {
		Con_Printf("No server to reconnect to...\n");
		return;
	}

	CL_Disconnect();
	CL_BeginServerConnect();
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket ()
{
	char *s;
	int c;

	MSG_BeginReading();
	MSG_ReadLong(); // skip the -1

	c = MSG_ReadByte();

	if (!cls.demoplayback)
	{
		Con_Printf("%s: ", NET_AdrToString(net_from));
	}

	if (c == S2C_CONNECTION)
	{
		Con_Printf("connection\n");
		if (cls.state >= ca_connected)
		{
			if (!cls.demoplayback)
			{
				Con_Printf("Dup connect received.  Ignored.\n");
			}
			return;
		}
		Netchan_Setup(&cls.netchan, net_from, CLIENT, cls.qport);
		MSG_WriteChar(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, "new");
		cls.state = ca_connected;
		Con_Printf("Connected.\n");
		return;
	}

	// remote command from gui front end
	if (c == A2C_CLIENT_COMMAND)
	{
		Con_Printf("client command\n");
		Con_Printf("Ignored.\n");
		return;
	}

	// print command from somewhere
	if (c == A2C_PRINT)
	{
		Con_Printf("print\n");

		s = MSG_ReadString();
		Con_Print(s);
		return;
	}

	// ping from somewhere
	if (c == A2A_PING)
	{
		char data[6];

		Con_Printf("ping\n");

		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;

		NET_SendPacket(CLIENT, 6, &data, net_from);
		return;
	}

	if (c == S2C_CHALLENGE)
	{
		Con_Printf("challenge\n");

		s = MSG_ReadString();
		cls.challenge = atoi(s);
		CL_SendConnectPacket();
		return;
	}

	Con_Printf("unknown:  %c\n", c);
}


/*
===============
CL_ReadPackets

Read all incoming data from the server
===============
*/
void CL_ReadPackets ()
{
	while (CL_GetMessage())
	{
		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket();
			continue;
		}

		if (net_message.cursize < 8)
		{
			Con_Printf("%s: Runt packet\n", NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!cls.demoplayback
		 && !NET_CompareAdr(net_from, cls.netchan.remote_address))
		{
			Con_DPrintf("%s: Sequenced packet without connection\n", NET_AdrToString(net_from));
			continue;
		}

		if (!Netchan_Process(&cls.netchan))
		{
			continue; // wasn't accepted for some reason
		}

		CL_ParseServerMessage();
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected && realtime - cls.netchan.last_received > cl_timeout.value)
	{
		Con_Printf("\nServer connection timed out.\n");
		CL_Disconnect();
		return;
	}
}

/*
=================
CL_Init
=================
*/
void CL_Init ()
{
	cls.state = ca_disconnected;

	Info_SetValueForKey (cls.userinfo, "name", "unnamed", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "topcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "bottomcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "rate", "2500", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "msg", "1", MAX_INFO_STRING);
	Info_SetValueForStarKey (cls.userinfo, "*ver", QUAKE_VERSION, MAX_INFO_STRING);

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitPrediction ();
	// CL_InitCam ();
	Pmove_Init ();
	
//
// register our commands
//
	Cvar_RegisterVariable (&cl_name);
	Cvar_RegisterVariable (&cl_color);
	Cvar_RegisterVariable (&cl_upspeed);
	Cvar_RegisterVariable (&cl_forwardspeed);
	Cvar_RegisterVariable (&cl_backspeed);
	Cvar_RegisterVariable (&cl_sidespeed);
	Cvar_RegisterVariable (&cl_movespeedkey);
	Cvar_RegisterVariable (&cl_yawspeed);
	Cvar_RegisterVariable (&cl_pitchspeed);
	Cvar_RegisterVariable (&cl_anglespeedkey);
	Cvar_RegisterVariable (&cl_timeout);
	Cvar_RegisterVariable (&cl_shownet);
	Cvar_RegisterVariable (&cl_nolerp);
	Cvar_RegisterVariable (&cl_sbar);
	Cvar_RegisterVariable (&cl_maxfps);
	Cvar_RegisterVariable (&cl_showfps);
	Cvar_RegisterVariable (&lookspring);
	Cvar_RegisterVariable (&lookstrafe);
	Cvar_RegisterVariable (&sensitivity);

	Cvar_RegisterVariable (&m_pitch);
	Cvar_RegisterVariable (&m_yaw);
	Cvar_RegisterVariable (&m_forward);
	Cvar_RegisterVariable (&m_side);

	Cvar_RegisterVariable (&entlatency);
	Cvar_RegisterVariable (&cl_predict_players2);
	Cvar_RegisterVariable (&cl_predict_players);
	Cvar_RegisterVariable (&cl_solid_players);
	
	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("rerecord", CL_ReRecord_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
}

