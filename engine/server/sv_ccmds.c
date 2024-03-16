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

bool sv_allow_cheats;

int fp_messages = 4, fp_persecond = 4, fp_secondsdead = 10;
char fp_msg[255] = {0};
extern cvar_t cl_warncmd;
extern redirect_t sv_redirected;
extern netadr_t master_adr[MAX_MASTERS];

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Make a master server current
====================
*/
static void SV_SetMaster_f ()
{
	char data[2];
	int i;

	memset(&master_adr, 0, sizeof(master_adr));

	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (!strcmp(Cmd_Argv(i), "none") || !NET_StringToAdr(Cmd_Argv(i), &master_adr[i - 1]))
		{
			Con_Printf("Setting nomaster mode.\n");
			return;
		}
		if (master_adr[i - 1].port == 0)
			master_adr[i - 1].port = BigShort(PORT_MASTER);

		Con_Printf("Master server at %s\n", NET_AdrToString(master_adr[i - 1]));

		Con_Printf("Sending a ping.\n");

		data[0] = A2A_PING;
		data[1] = 0;
		NET_SendPacket(SERVER, 2, data, master_adr[i - 1]);
	}

	svs.last_heartbeat = -99999;
}

/*
============
SV_Logfile_f

Toodles FIXME:
============
*/
static void SV_Logfile_f ()
{
}


/*
============
SV_Fraglogfile_f
============
*/
static void SV_Fraglogfile_f ()
{
	char	name[MAX_OSPATH];
	int		i;

	if (sv_fraglogfile)
	{
		Con_Printf ("Frag file logging off.\n");
		fclose (sv_fraglogfile);
		sv_fraglogfile = NULL;
		return;
	}

	// find an unused name
	for (i=0 ; i<1000 ; i++)
	{
		sprintf (name, "%s/frag_%i.log", com_gamedir, i);
		sv_fraglogfile = fopen (name, "r");
		if (!sv_fraglogfile)
		{	// can't read it, so create this one
			sv_fraglogfile = fopen (name, "w");
			if (!sv_fraglogfile)
				i=1000;	// give error
			break;
		}
		fclose (sv_fraglogfile);
	}
	if (i==1000)
	{
		Con_Printf ("Can't open any logfiles.\n");
		sv_fraglogfile = NULL;
		return;
	}

	Con_Printf ("Logging frags to %s.\n", name);
}


/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
static bool SV_SetPlayer ()
{
	client_t	*cl;
	int			i;
	int			idnum;

	idnum = atoi(Cmd_Argv(1));

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == idnum
		 || !strcasecmp(cl->name, Cmd_Argv(1)))
		{
			host_client = cl;
			sv_player = host_client->edict;
			return true;
		}
	}
	Con_Printf ("Userid %i is not on the server\n", idnum);
	return false;
}


/*
==================
SV_God_f

Sets client to godmode
==================
*/
static void SV_God_f ()
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	ed_float(sv_player, flags) = (int)ed_float(sv_player, flags) ^ FL_GODMODE;
	if (!((int)ed_float(sv_player, flags) & FL_GODMODE) )
		SV_ClientPrintf (host_client, PRINT_HIGH, "godmode OFF\n");
	else
		SV_ClientPrintf (host_client, PRINT_HIGH, "godmode ON\n");
}

static void SV_Notarget_f ()
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	ed_float(sv_player, flags) = (int)ed_float(sv_player, flags) ^ FL_NOTARGET;
	if (!((int)ed_float(sv_player, flags) & FL_NOTARGET) )
		SV_ClientPrintf (host_client, PRINT_HIGH, "notarget OFF\n");
	else
		SV_ClientPrintf (host_client, PRINT_HIGH, "notarget ON\n");
}


static void SV_Noclip_f ()
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	if (ed_float(sv_player, movetype) != MOVETYPE_NOCLIP)
	{
		ed_float(sv_player, movetype) = MOVETYPE_NOCLIP;
		SV_ClientPrintf (host_client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		ed_float(sv_player, movetype) = MOVETYPE_WALK;
		SV_ClientPrintf (host_client, PRINT_HIGH, "noclip OFF\n");
	}
}


/*
==================
SV_Give_f
==================
*/
static void SV_Give_f ()
{
	char	*t;
	int		v;
	
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}
	
	if (!SV_SetPlayer ())
		return;

	t = Cmd_Argv(2);
	v = atoi (Cmd_Argv(3));
	
	switch (t[0])
	{
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      if (hipnotic)
      {
         if (t[0] == '6')
         {
            if (t[1] == 'a')
               ed_float(sv_player, items) = (int)ed_float(sv_player, items) | HIT_PROXIMITY_GUN;
            else
               ed_float(sv_player, items) = (int)ed_float(sv_player, items) | IT_GRENADE_LAUNCHER;
         }
         else if (t[0] == '9')
            ed_float(sv_player, items) = (int)ed_float(sv_player, items) | HIT_LASER_CANNON;
         else if (t[0] == '0')
            ed_float(sv_player, items) = (int)ed_float(sv_player, items) | HIT_MJOLNIR;
         else if (t[0] >= '2')
            ed_float(sv_player, items) = (int)ed_float(sv_player, items) | (IT_SHOTGUN << (t[0] - '2'));
      }
      else
      {
         if (t[0] >= '2')
            ed_float(sv_player, items) = (int)ed_float(sv_player, items) | (IT_SHOTGUN << (t[0] - '2'));
      }
		break;
	
    case 's':
		if (ed_field(ammo_shells1))
		{
			ed_float(sv_player, ammo_shells1) = v;
		}

        ed_float(sv_player, ammo_shells) = v;
        break;		
    case 'n':
		if (ed_field(ammo_nails1))
		{
			ed_float(sv_player, ammo_nails1) = v;

			if (ed_float(sv_player, weapon) <= IT_LIGHTNING)
				ed_float(sv_player, ammo_nails) = v;
		}
		else
		{
			ed_float(sv_player, ammo_nails) = v;
		}
        break;		
    case 'l':
		if (ed_field(ammo_lava_nails))
		{
			ed_float(sv_player, ammo_lava_nails) = v;
			
			if (ed_float(sv_player, weapon) > IT_LIGHTNING)
				ed_float(sv_player, ammo_nails) = v;
		}
        break;
    case 'r':
		if (ed_field(ammo_rockets1))
		{
			ed_float(sv_player, ammo_rockets1) = v;
			
			if (ed_float(sv_player, weapon) <= IT_LIGHTNING)
				ed_float(sv_player, ammo_rockets) = v;
		}
		else
		{
			ed_float(sv_player, ammo_rockets) = v;
		}
        break;		
    case 'm':
		if (ed_field(ammo_multi_rockets))
		{
			ed_float(sv_player, ammo_multi_rockets) = v;
			
				if (ed_float(sv_player, weapon) > IT_LIGHTNING)
					ed_float(sv_player, ammo_rockets) = v;
		}
        break;		
    case 'h':
        ed_float(sv_player, health) = v;
        break;		
    case 'c':
		if (ed_field(ammo_cells1))
		{
			ed_float(sv_player, ammo_cells1) = v;
			
			if (ed_float(sv_player, weapon) <= IT_LIGHTNING)
				ed_float(sv_player, ammo_cells) = v;
		}
		else
		{
			ed_float(sv_player, ammo_cells) = v;
		}
        break;		
    case 'p':
		if (ed_field(ammo_plasma))
		{
			ed_float(sv_player, ammo_plasma) = v;
			
			if (ed_float(sv_player, weapon) > IT_LIGHTNING)
				ed_float(sv_player, ammo_cells) = v;
		}
        break;		
    }
}


/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
static void SV_Kick_f ()
{
	int			i;
	client_t	*cl;
	int			uid;

	uid = atoi(Cmd_Argv(1));
	
	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == uid
		 || !strcasecmp(cl->name, Cmd_Argv(1)))
		{
			SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", cl->name);
			// print directly, because the dropped client won't get the
			// SV_BroadcastPrintf message
			SV_ClientPrintf (cl, PRINT_HIGH, "You were kicked from the game\n");
			SV_DropClient (cl); 
			return;
		}
	}

	Con_Printf ("Couldn't find user number %i\n", uid);
}


/*
================
SV_Status_f
================
*/
void SV_Status_f ()
{
	int			i, j, l;
	client_t	*cl;
	float		cpu, avg, pak;
	char		*s;


	cpu = (svs.stats.latched_active+svs.stats.latched_idle);
	if (cpu)
		cpu = 100*svs.stats.latched_active/cpu;
	avg = 1000*svs.stats.latched_active / STATFRAMES;
	pak = (float)svs.stats.latched_packets/ STATFRAMES;

	Con_Printf ("net address      : %s\n",NET_AdrToString (NET_GetLocalAddress ()));
	Con_Printf ("cpu utilization  : %3i%%\n",(int)cpu);
	Con_Printf ("avg response time: %i ms\n",(int)avg);
	Con_Printf ("packets/frame    : %5.2f (%d)\n", pak, num_prstr);
	
// min fps lat drp
	if (sv_redirected != RD_NONE) {
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		Con_Printf ("name               userid frags\n");
        Con_Printf ("  address          rate ping drop\n");
		Con_Printf ("  ---------------- ---- ---- -----\n");
		for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (!cl->state)
				continue;

			Con_Printf ("%-16.16s  ", cl->name);

			Con_Printf ("%6i %5i", cl->userid, (int)ed_float(cl->edict, frags));
			if (cl->spectator)
				Con_Printf(" (s)\n");
			else			
				Con_Printf("\n");

			s = NET_BaseAdrToString ( cl->netchan.remote_address);
			Con_Printf ("  %-16.16s", s);
			if (cl->state == cs_connected)
			{
				Con_Printf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie)
			{
				Con_Printf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %5.2f\n"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence);
		}
	} else {
		Con_Printf ("frags userid address         name            rate ping drop  qport\n");
		Con_Printf ("----- ------ --------------- --------------- ---- ---- ----- -----\n");
		for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (!cl->state)
				continue;
			Con_Printf ("%5i %6i ", (int)ed_float(cl->edict, frags),  cl->userid);

			s = NET_BaseAdrToString ( cl->netchan.remote_address);
			Con_Printf ("%s", s);
			l = 16 - strlen(s);
			for (j=0 ; j<l ; j++)
				Con_Printf (" ");
			
			Con_Printf ("%s", cl->name);
			l = 16 - strlen(cl->name);
			for (j=0 ; j<l ; j++)
				Con_Printf (" ");
			if (cl->state == cs_connected)
			{
				Con_Printf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie)
			{
				Con_Printf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %3.1f %4i"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence
				, cl->netchan.qport);
			if (cl->spectator)
				Con_Printf(" (s)\n");
			else			
				Con_Printf("\n");

				
		}
	}
	Con_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f()
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
static void SV_Heartbeat_f ()
{
	svs.last_heartbeat = -9999;
}

void SV_SendServerInfoChange(char *key, char *value)
{
	if (!sv.state)
		return;

	MSG_WriteByte (&sv.reliable_datagram, svc_serverinfo);
	MSG_WriteString (&sv.reliable_datagram, key);
	MSG_WriteString (&sv.reliable_datagram, value);
}

/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
char *CopyString(char *s);
static void SV_Serverinfo_f ()
{
	cvar_t	*var;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Server info settings:\n");
		Info_Print (svs.info);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey (svs.info, Cmd_Argv(1), Cmd_Argv(2), MAX_SERVERINFO_STRING, sv_highchars.value);

	// if this is a cvar, change it too	
	var = Cvar_FindVar (src_server, Cmd_Argv(1));
	if (var)
	{
		Z_Free (var->string);	// free the old value string	
		var->string = CopyString (Cmd_Argv(2));
		var->value = atof (var->string);
	}

	SV_SendServerInfoChange(Cmd_Argv(1), Cmd_Argv(2));
}


/*
===========
SV_Localinfo_f

  Examine or change the serverinfo string
===========
*/
char *CopyString(char *s);
static void SV_Localinfo_f ()
{
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Local info settings:\n");
		Info_Print (localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey (localinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_LOCALINFO_STRING, sv_highchars.value);
}


/*
===========
SV_User_f

Examine a users info strings
===========
*/
static void SV_User_f ()
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Info_Print (host_client->userinfo);
}

/*
================
SV_Gamedir

Sets the fake *gamedir to a different directory.
================
*/
static void SV_Gamedir ()
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current *gamedir: %s\n", Info_ValueForKey (svs.info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("*Gamedir should be a single filename, not a path\n");
		return;
	}

	Info_SetValueForStarKey (svs.info, "*gamedir", dir, MAX_SERVERINFO_STRING, sv_highchars.value);
}

/*
================
SV_Floodport_f

Sets the gamedir and path to a different directory.
================
*/

static void SV_Floodprot_f ()
{
	int arg1, arg2, arg3;
	
	if (Cmd_Argc() == 1)
	{
		if (fp_messages) {
			Con_Printf ("Current floodprot settings: \nAfter %d msgs per %d seconds, silence for %d seconds\n", 
				fp_messages, fp_persecond, fp_secondsdead);
			return;
		} else
			Con_Printf ("No floodprots enabled.\n");
	}

	if (Cmd_Argc() != 4)
	{
		Con_Printf ("Usage: floodprot <# of messages> <per # of seconds> <seconds to silence>\n");
		Con_Printf ("Use floodprotmsg to set a custom message to say to the flooder.\n");
		return;
	}

	arg1 = atoi(Cmd_Argv(1));
	arg2 = atoi(Cmd_Argv(2));
	arg3 = atoi(Cmd_Argv(3));

	if (arg1<=0 || arg2 <= 0 || arg3<=0) {
		Con_Printf ("All values must be positive numbers\n");
		return;
	}
	
	if (arg1 > 10) {
		Con_Printf ("Can only track up to 10 messages.\n");
		return;
	}

	fp_messages	= arg1;
	fp_persecond = arg2;
	fp_secondsdead = arg3;
}

static void SV_Floodprotmsg_f ()
{
	if (Cmd_Argc() == 1) {
		Con_Printf("Current msg: %s\n", fp_msg);
		return;
	} else if (Cmd_Argc() != 2) {
		Con_Printf("Usage: floodprotmsg \"<message>\"\n");
		return;
	}
	sprintf(fp_msg, "%s", Cmd_Argv(1));
}
  
/*
================
SV_Gamedir_f

Sets the gamedir and path to a different directory.
================
*/
extern char	gamedirfile[];
static void SV_Gamedir_f ()
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current gamedir: %s\n", com_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: gamedir <newdir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	COM_Gamedir (dir);
	Info_SetValueForStarKey (svs.info, "*gamedir", dir, MAX_SERVERINFO_STRING, sv_highchars.value);
}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands ()
{
	if (COM_CheckParm ("-cheats"))
	{
		sv_allow_cheats = true;
		Info_SetValueForStarKey (svs.info, "*cheats", "ON", MAX_SERVERINFO_STRING, sv_highchars.value);
	}

	Cmd_AddCommand (src_server, "logfile", SV_Logfile_f);
	Cmd_AddCommand (src_server, "fraglogfile", SV_Fraglogfile_f);

	Cmd_AddCommand (src_server, "kick", SV_Kick_f);
	Cmd_AddCommand (src_server, "status", SV_Status_f);

	Cmd_AddCommand (src_server, "setmaster", SV_SetMaster_f);

	Cmd_AddCommand (src_server, "say", SV_ConSay_f);
	Cmd_AddCommand (src_server, "heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand (src_server, "god", SV_God_f);
	Cmd_AddCommand (src_server, "notarget", SV_Notarget_f);
	Cmd_AddCommand (src_server, "give", SV_Give_f);
	Cmd_AddCommand (src_server, "noclip", SV_Noclip_f);
	Cmd_AddCommand (src_server, "serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand (src_server, "localinfo", SV_Localinfo_f);
	Cmd_AddCommand (src_server, "user", SV_User_f);
	Cmd_AddCommand (src_server, "gamedir", SV_Gamedir_f);
	Cmd_AddCommand (src_server, "sv_gamedir", SV_Gamedir);
	Cmd_AddCommand (src_server, "floodprot", SV_Floodprot_f);
	Cmd_AddCommand (src_server, "floodprotmsg", SV_Floodprotmsg_f);

	cl_warncmd.value = 1;
}
