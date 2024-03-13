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

cvar_t	rcon_password = {"rcon_password", ""};
cvar_t	rcon_address = {"rcon_address", ""};

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

//
// info mirrors
//
cvar_t	password = {"password", "", CVAR_CLIENT_INFO};
cvar_t	spectator = {"spectator", "", CVAR_CLIENT_INFO};
cvar_t	name = {"name", "unnamed", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	team = {"team","", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	skin = {"skin","", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	topcolor = {"topcolor","0", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	bottomcolor = {"bottomcolor","0", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	rate = {"rate","2500", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	noaim = {"noaim","0", CVAR_ARCHIVE | CVAR_CLIENT_INFO};
cvar_t	msg = {"msg","1", CVAR_ARCHIVE | CVAR_CLIENT_INFO};

client_static_t	cls;
client_state_t	cl;

entity_state_t	cl_baselines[MIN_EDICTS];
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_numvisedicts, cl_oldnumvisedicts;
entity_t		*cl_visedicts, *cl_oldvisedicts;
entity_t		cl_visedicts_list[2][MAX_VISEDICTS];

static double connect_time = -1; // for connection retransmits

float server_version = 0; // version of server we connected to

char emodel_name[] = 
	{ 'e' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char pmodel_name[] = 
	{ 'p' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char prespawn_name[] = 
	{ 'p'^0xff, 'r'^0xff, 'e'^0xff, 's'^0xff, 'p'^0xff, 'a'^0xff, 'w'^0xff, 'n'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '0'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char modellist_name[] = 
	{ 'm'^0xff, 'o'^0xff, 'd'^0xff, 'e'^0xff, 'l'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff, 
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char soundlist_name[] = 
	{ 's'^0xff, 'o'^0xff, 'u'^0xff, 'n'^0xff, 'd'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff, 
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };

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

	Info_SetValueForStarKey(cls.userinfo, "*ip", NET_AdrToString(adr), MAX_INFO_STRING, true);

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
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
static void CL_Rcon_f ()
{
	char	message[1024];
	int		i;
	netadr_t	to;

	if (!rcon_password.string)
	{
		Con_Printf ("You must set 'rcon_password' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = 255;
	message[1] = 255;
	message[2] = 255;
	message[3] = 255;
	message[4] = 0;

	strcat (message, "rcon ");

	strcat (message, rcon_password.string);
	strcat (message, " ");

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		strcat (message, Cmd_Argv(i));
		strcat (message, " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.string))
		{
			Con_Printf ("You must either be connected,\n"
						"or set the 'rcon_address' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rcon_address.string, &to);
	}
	
	NET_SendPacket (CLIENT, strlen(message)+1, message
		, to);
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
		/* Toodles: Completely kill the connection to prevent unwanted packets. */
		memset (&cls.netchan, 0, sizeof (cls.netchan));

		cls.state = ca_disconnected;
		if (Host_IsLocalGame ())
			Host_ShutdownServer(false);

		cls.demoplayback = cls.demorecording = cls.timedemo = false;
	}
	Cam_Reset();

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
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
static void CL_User_f (void)
{
	int		uid;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: user <username / userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.players[i].name[0])
			continue;
		if (cl.players[i].userid == uid
		|| !strcmp(cl.players[i].name, Cmd_Argv(1)) )
		{
			Info_Print (cl.players[i].userinfo);
			return;
		}
	}
	Con_Printf ("User not in server.\n");
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
static void CL_Users_f (void)
{
	int		i;
	int		c;

	c = 0;
	Con_Printf ("userid frags name\n");
	Con_Printf ("------ ----- ----\n");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0])
		{
			Con_Printf ("%6i %4i %s\n", cl.players[i].userid, cl.players[i].frags, cl.players[i].name);
			c++;
		}
	}

	Con_Printf ("%i total users\n", c);
}

void CL_Color_f (void)
{
	// just for quake compatability...
	int		top, bottom;
	char	num[16];

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"color\" is \"%s %s\"\n",
			Info_ValueForKey (cls.userinfo, "topcolor"),
			Info_ValueForKey (cls.userinfo, "bottomcolor") );
		Con_Printf ("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		top = bottom = atoi(Cmd_Argv(1));
	else
	{
		top = atoi(Cmd_Argv(1));
		bottom = atoi(Cmd_Argv(2));
	}
	
	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;
	
	sprintf (num, "%i", top);
	Cvar_Set (src_client, "topcolor", num);
	sprintf (num, "%i", bottom);
	Cvar_Set (src_client, "bottomcolor", num);
}

/*
==================
CL_FullServerinfo_f

Sent by server when serverinfo changes
==================
*/
static void CL_FullServerinfo_f (void)
{
	char *p;
	float v;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: fullserverinfo <complete info string>\n");
		return;
	}

	strcpy (cl.serverinfo, Cmd_Argv(1));

	if ((p = Info_ValueForKey(cl.serverinfo, "*vesion")) && *p) {
		v = atof(p);
		if (v) {
			if (!server_version)
				Con_Printf("Version %1.2f Server\n", v);
			server_version = v;
		}
	}
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
*/
static void CL_FullInfo_f (void)
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Con_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		if (!strcasecmp(key, pmodel_name) || !strcasecmp(key, emodel_name))
			continue;

		Info_SetValueForKey (cls.userinfo, key, value, MAX_INFO_STRING, true);
	}
}

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f (void)
{
	if (Cmd_Argc() == 1)
	{
		Info_Print (cls.userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	if (!strcasecmp(Cmd_Argv(1), pmodel_name) || !strcmp(Cmd_Argv(1), emodel_name))
		return;

	Info_SetValueForKey (cls.userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING, true);
	if (cls.state >= ca_connected)
		Cmd_ForwardToServer ();
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (void)
{
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
	Cbuf_InsertText (src_client, str);
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
		if (*(int32_t *)net_message.data == -1)
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

//=============================================================================

/*
=====================
CL_Download_f
=====================
*/
static void CL_Download_f (void)
{
	char *p, *q;

	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Must be connected.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: download <datafile>\n");
		return;
	}

	sprintf (cls.downloadname, "%s/%s", com_gamedir, Cmd_Argv(1));

	p = cls.downloadname;
	for (;;) {
		if ((q = strchr(p, '/')) != NULL) {
			*q = 0;
			Sys_mkdir(cls.downloadname);
			*q = '/';
			p = q + 1;
		} else
			break;
	}

	strcpy(cls.downloadtempname, cls.downloadname);
	cls.download = fopen (cls.downloadname, "wb");
	cls.downloadtype = dl_single;

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, va("download %s\n",Cmd_Argv(1)));
}

static void simple_crypt(char *buf, int len)
{
	while (len--)
		*buf++ ^= 0xff;
}

static void CL_FixupModelNames()
{
	simple_crypt(emodel_name, sizeof(emodel_name) - 1);
	simple_crypt(pmodel_name, sizeof(pmodel_name) - 1);
	simple_crypt(prespawn_name,  sizeof(prespawn_name)  - 1);
	simple_crypt(modellist_name, sizeof(modellist_name) - 1);
	simple_crypt(soundlist_name, sizeof(soundlist_name) - 1);
}

/*
=================
CL_Init
=================
*/
void CL_Init ()
{
	extern cvar_t baseskin;
	extern cvar_t noskins;

	cls.state = ca_disconnected;

	CL_FixupModelNames();

	Info_SetValueForKey (cls.userinfo, "name", "unnamed", MAX_INFO_STRING, true);
	Info_SetValueForKey (cls.userinfo, "topcolor", "0", MAX_INFO_STRING, true);
	Info_SetValueForKey (cls.userinfo, "bottomcolor", "0", MAX_INFO_STRING, true);
	Info_SetValueForKey (cls.userinfo, "rate", "2500", MAX_INFO_STRING, true);
	Info_SetValueForKey (cls.userinfo, "msg", "1", MAX_INFO_STRING, true);

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitPrediction ();
	CL_InitCam ();
	Pmove_Init ();
	
//
// register our commands
//
	Cvar_RegisterVariable (src_client, &cl_upspeed);
	Cvar_RegisterVariable (src_client, &cl_forwardspeed);
	Cvar_RegisterVariable (src_client, &cl_backspeed);
	Cvar_RegisterVariable (src_client, &cl_sidespeed);
	Cvar_RegisterVariable (src_client, &cl_movespeedkey);
	Cvar_RegisterVariable (src_client, &cl_yawspeed);
	Cvar_RegisterVariable (src_client, &cl_pitchspeed);
	Cvar_RegisterVariable (src_client, &cl_anglespeedkey);
	Cvar_RegisterVariable (src_client, &cl_shownet);
	Cvar_RegisterVariable (src_client, &cl_nolerp);
	Cvar_RegisterVariable (src_client, &cl_sbar);
	Cvar_RegisterVariable (src_client, &cl_maxfps);
	Cvar_RegisterVariable (src_client, &cl_showfps);
	Cvar_RegisterVariable (src_client, &cl_timeout);
	Cvar_RegisterVariable (src_client, &lookspring);
	Cvar_RegisterVariable (src_client, &lookstrafe);
	Cvar_RegisterVariable (src_client, &sensitivity);

	Cvar_RegisterVariable (src_client, &m_pitch);
	Cvar_RegisterVariable (src_client, &m_yaw);
	Cvar_RegisterVariable (src_client, &m_forward);
	Cvar_RegisterVariable (src_client, &m_side);

	Cvar_RegisterVariable (src_client, &rcon_password);
	Cvar_RegisterVariable (src_client, &rcon_address);

	Cvar_RegisterVariable (src_client, &entlatency);
	Cvar_RegisterVariable (src_client, &cl_predict_players2);
	Cvar_RegisterVariable (src_client, &cl_predict_players);
	Cvar_RegisterVariable (src_client, &cl_solid_players);

	Cvar_RegisterVariable (src_client, &baseskin);
	Cvar_RegisterVariable (src_client, &noskins);

	//
	// info mirrors
	//
	Cvar_RegisterVariable (src_client, &name);
	Cvar_RegisterVariable (src_client, &password);
	Cvar_RegisterVariable (src_client, &spectator);
	Cvar_RegisterVariable (src_client, &skin);
	Cvar_RegisterVariable (src_client, &team);
	Cvar_RegisterVariable (src_client, &topcolor);
	Cvar_RegisterVariable (src_client, &bottomcolor);
	Cvar_RegisterVariable (src_client, &rate);
	Cvar_RegisterVariable (src_client, &msg);
	Cvar_RegisterVariable (src_client, &noaim);

	Cmd_AddCommand (src_client, "changing", CL_Changing_f);
	Cmd_AddCommand (src_client, "disconnect", CL_Disconnect_f);
	Cmd_AddCommand (src_client, "record", CL_Record_f);
	Cmd_AddCommand (src_client, "rerecord", CL_ReRecord_f);
	Cmd_AddCommand (src_client, "stop", CL_Stop_f);
	Cmd_AddCommand (src_client, "playdemo", CL_PlayDemo_f);
	Cmd_AddCommand (src_client, "timedemo", CL_TimeDemo_f);

	Cmd_AddCommand (src_client, "skins", Skin_Skins_f);
	Cmd_AddCommand (src_client, "allskins", Skin_AllSkins_f);

	Cmd_AddCommand (src_client, "connect", CL_Connect_f);
	Cmd_AddCommand (src_client, "reconnect", CL_Reconnect_f);

	Cmd_AddCommand (src_client, "rcon", CL_Rcon_f);
	Cmd_AddCommand (src_client, "packet", CL_Packet_f);
	Cmd_AddCommand (src_client, "user", CL_User_f);
	Cmd_AddCommand (src_client, "users", CL_Users_f);

	Cmd_AddCommand (src_client, "setinfo", CL_SetInfo_f);
	Cmd_AddCommand (src_client, "fullinfo", CL_FullInfo_f);
	Cmd_AddCommand (src_client, "fullserverinfo", CL_FullServerinfo_f);

	Cmd_AddCommand (src_client, "color", CL_Color_f);
	Cmd_AddCommand (src_client, "download", CL_Download_f);

	Cmd_AddCommand (src_client, "nextul", CL_NextUpload);
	Cmd_AddCommand (src_client, "stopul", CL_StopUpload);
}

