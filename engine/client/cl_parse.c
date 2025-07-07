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

int parsecountmod;
double parsecounttime;

int cl_spikeindex, cl_playerindex, cl_flagindex;

int packet_latency[NET_TIMINGS];

int CL_CalcNet (void)
{
	int a, i;
	frame_t *frame;
	int lost;
	char st[80];

	for (i = cls.netchan.outgoing_sequence - UPDATE_BACKUP + 1; i <= cls.netchan.outgoing_sequence; i++)
	{
		frame = &cl.frames[i & UPDATE_MASK];
		if (frame->receivedtime == -1)
			packet_latency[i & NET_TIMINGSMASK] = 9999; // dropped
		else if (frame->receivedtime == -2)
			packet_latency[i & NET_TIMINGSMASK] = 10000; // choked
		else if (frame->invalid)
			packet_latency[i & NET_TIMINGSMASK] = 9998; // invalid delta
		else
			packet_latency[i & NET_TIMINGSMASK] = (frame->receivedtime - frame->senttime) * 20;
	}

	lost = 0;
	for (a = 0; a < NET_TIMINGS; a++)
	{
		i = (cls.netchan.outgoing_sequence - a) & NET_TIMINGSMASK;
		if (packet_latency[i] == 9999)
			lost++;
	}
	return lost * 100 / NET_TIMINGS;
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
bool CL_CheckOrDownloadFile (char *filename)
{
	FILE *f;

	if (strstr (filename, ".."))
	{
		Con_Printf ("Refusing to download a path with ..\n");
		return true;
	}

	COM_FOpenFile (filename, &f);
	if (f)
	{ // it exists, no need to download
		fclose (f);
		return true;
	}

	//ZOID - can't download when recording
	if (cls.demorecording)
	{
		Con_Printf ("Unable to download %s in record mode.\n", cls.downloadname);
		return true;
	}
	//ZOID - can't download when playback
	if (cls.demoplayback)
		return true;

	strcpy (cls.downloadname, filename);
	Con_Printf ("Downloading %s...\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va ("download %s", cls.downloadname));

	cls.downloadnumber++;

	return false;
}

static void Model_NextDownload (void)
{
	char *s;
	int i;
	extern char gamedirfile[];

	if (cls.downloadnumber == 0)
	{
		Con_Printf ("Checking models...\n");
		cls.downloadnumber = 1;
	}

	cls.downloadtype = dl_model;
	for (; cl.model_name[cls.downloadnumber][0]; cls.downloadnumber++)
	{
		s = cl.model_name[cls.downloadnumber];
		if (s[0] == '*')
			continue; // inline brush model
		if (!CL_CheckOrDownloadFile (s))
			return; // started a download
	}

	for (i = 1; i < MAX_MODELS; i++)
	{
		if (!cl.model_name[i][0])
			break;

		cl.model_precache[i] = Mod_ForName (cl.model_name[i], false, i == 1);
		cl.cmodel_precache[i] = CMod_ForName (cl.model_name[i], false, i == 1);

		if (!cl.model_precache[i])
		{
			Con_Printf ("\nThe required model file '%s' could not be found or downloaded.\n\n", cl.model_name[i]);
			Con_Printf ("You may need to download or purchase a %s client "
						"pack in order to play on this server.\n\n",
						gamedirfile);
			CL_Disconnect ();
			return;
		}
	}

	CL_InitTEnts ();

	// all done
	cl.worldmodel = cl.model_precache[1];
	R_NewMap ();
	Hunk_Check (); // make sure nothing is hurt

	// done with modellist, request first of static signon messages
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va (prespawn_name, cl.servercount, cl.cmodel_precache[1]->checksum2));
}

static void Sound_NextDownload (void)
{
	char *s;
	int i;

	if (cls.downloadnumber == 0)
	{
		Con_Printf ("Checking sounds...\n");
		cls.downloadnumber = 1;
	}

	cls.downloadtype = dl_sound;
	for (; cl.sound_name[cls.downloadnumber][0]; cls.downloadnumber++)
	{
		s = cl.sound_name[cls.downloadnumber];
		if (!CL_CheckOrDownloadFile (va ("sound/%s", s)))
			return; // started a download
	}

	for (i = 1; i < MAX_SOUNDS; i++)
	{
		if (!cl.sound_name[i][0])
			break;
		cl.sound_precache[i] = S_PrecacheSound (cl.sound_name[i]);
	}

	// done with sounds, request models now
	memset (cl.model_precache, 0, sizeof (cl.model_precache));
	memset (cl.cmodel_precache, 0, sizeof (cl.cmodel_precache));
	cl_playerindex = -1;
	cl_spikeindex = -1;
	cl_flagindex = -1;
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	//	MSG_WriteString (&cls.netchan.message, va("modellist %i 0", cl.servercount));
	MSG_WriteString (&cls.netchan.message, va (modellist_name, cl.servercount, 0));
}

static void CL_RequestNextDownload (void)
{
	switch (cls.downloadtype)
	{
	case dl_single:
		break;
	case dl_model:
		Model_NextDownload ();
		break;
	case dl_sound:
		Sound_NextDownload ();
		break;
	case dl_none:
	default:
		Con_DPrintf ("Unknown download type.\n");
	}
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
static void CL_ParseDownload (void)
{
	int size, percent;
	byte name[1024];
	int r;

	// read the data
	size = MSG_ReadShort ();
	percent = MSG_ReadByte ();

	if (cls.demoplayback)
	{
		if (size > 0)
			msg_readcount += size;
		return; // not in demo playback
	}

	if (size == -1)
	{
		Con_Printf ("File not found.\n");
		if (cls.download)
		{
			Con_Printf ("cls.download shouldn't have been set\n");
			fclose (cls.download);
			cls.download = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		sprintf (name, "%s/%s", com_gamedir, cls.downloadtempname);

		COM_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			msg_readcount += size;
			Con_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	fwrite (net_message[CLIENT].data + msg_readcount, 1, size, cls.download);
	msg_readcount += size;

	if (percent != 100)
	{
// change display routines by zoid
// request next block
#if 0
		Con_Printf (".");
		if (10*(percent/10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10*(percent/10);
			Con_Printf ("%i%%", cls.downloadpercent);
		}
#endif
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "nextdl");
	}
	else
	{
		char oldn[MAX_OSPATH];
		char newn[MAX_OSPATH];

#if 0
		Con_Printf ("100%%\n");
#endif

		fclose (cls.download);

		// rename the temp file to it's final name
		if (strcmp (cls.downloadtempname, cls.downloadname))
		{
			sprintf (oldn, "%s/%s", com_gamedir, cls.downloadtempname);
			sprintf (newn, "%s/%s", com_gamedir, cls.downloadname);
			r = rename (oldn, newn);
			if (r)
				Con_Printf ("failed to rename.\n");
		}

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

static byte *upload_data;
static int upload_pos;
static int upload_size;

void CL_NextUpload (void)
{
	byte buffer[1024];
	int r;
	int percent;
	int size;

	if (!upload_data)
		return;

	r = upload_size - upload_pos;
	if (r > 768)
		r = 768;
	memcpy (buffer, upload_data + upload_pos, r);
	MSG_WriteByte (&cls.netchan.message, clc_upload);
	MSG_WriteShort (&cls.netchan.message, r);

	upload_pos += r;
	size = upload_size;
	if (!size)
		size = 1;
	percent = upload_pos * 100 / size;
	MSG_WriteByte (&cls.netchan.message, percent);
	SZ_Write (&cls.netchan.message, buffer, r);

	Con_DPrintf ("UPLOAD: %6d: %d written\n", upload_pos - r, r);

	if (upload_pos != upload_size)
		return;

	Con_Printf ("Upload completed\n");

	free (upload_data);
	upload_data = 0;
	upload_pos = upload_size = 0;
}

void CL_StartUpload (byte *data, int size)
{
	if (cls.state < ca_onserver)
		return; // gotta be connected

	// override
	if (upload_data)
		free (upload_data);

	Con_DPrintf ("Upload starting of %d...\n", size);

	upload_data = malloc (size);
	memcpy (upload_data, data, size);
	upload_size = size;
	upload_pos = 0;

	CL_NextUpload ();
}

void CL_StopUpload (void)
{
	if (upload_data)
		free (upload_data);
	upload_data = NULL;
}

static void CL_ParseServerData (void)
{
	char *str;
	FILE *f;
	char fn[MAX_OSPATH];
	bool cflag = false;
	extern char gamedirfile[MAX_OSPATH];

	Con_DPrintf ("Serverdata packet received.\n");
	//
	// wipe the client_state_t struct
	//
	CL_ClearState ();

	// parse protocol version number
	int version = MSG_ReadLong ();

	if (!cls.demoplayback && version != PROTOCOL_VERSION)
		Host_Error ("Server returned version %i, not %i\n", version, PROTOCOL_VERSION);

	cls.protocol = MSG_ReadLong ();

	if (!cls.demoplayback && cls.protocol != PROTOCOL_NETQUAKE && cls.protocol != PROTOCOL_QUAKEWORLD)
		Host_Error ("Server using protocol %i\n", cls.protocol);

	Con_DPrintf ("Using protocol %i\n", cls.protocol);

	cl.servercount = MSG_ReadLong ();

	// game directory
	str = MSG_ReadString ();

	if (strcasecmp (gamedirfile, str))
	{
		// save current config
		Host_WriteConfiguration ();
		cflag = true;
	}

	COM_Gamedir (str);

	//ZOID--run the autoexec.cfg in the gamedir
	//if it exists
	if (cflag)
	{
		sprintf (fn, "%s/%s", com_gamedir, "config.cfg");
		if ((f = fopen (fn, "r")) != NULL)
		{
			fclose (f);
			Cbuf_AddText (src_client, "exec config.cfg\n");
			Cbuf_AddText (src_client, "exec frontend.cfg\n");
		}
	}

	// parse player slot
	cl.playernum = MSG_ReadByte ();

	// get the full level name
	str = MSG_ReadString ();
	strncpy (cl.levelname, str, sizeof (cl.levelname) - 1);

	// get the movevars
	movevars.gravity = MSG_ReadFloat ();
	movevars.stopspeed = MSG_ReadFloat ();
	movevars.maxspeed = MSG_ReadFloat ();
	movevars.accelerate = MSG_ReadFloat ();
	movevars.airaccelerate = MSG_ReadFloat ();
	movevars.wateraccelerate = MSG_ReadFloat ();
	movevars.friction = MSG_ReadFloat ();
	movevars.waterfriction = MSG_ReadFloat ();
	movevars.entgravity = MSG_ReadFloat ();

	// seperate the printfs so the server message can have a color
	Con_Printf ("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, cl.levelname);

	// ask for the sound list next
	memset (cl.sound_name, 0, sizeof (cl.sound_name));
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	//	MSG_WriteString (&cls.netchan.message, va("soundlist %i 0", cl.servercount));
	MSG_WriteString (&cls.netchan.message, va (soundlist_name, cl.servercount, 0));

	// now waiting for downloads, etc
	cls.state = ca_onserver;
}

static void CL_ParseSoundList (void)
{
	int numsounds;
	char *str;
	int n;

	// precache sounds
	//	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));

	numsounds = MSG_ReadShort ();

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		numsounds++;
		if (numsounds == MAX_SOUNDS)
			Host_Error ("Server sent too many sound_precache");
		strcpy (cl.sound_name[numsounds], str);
	}

	n = MSG_ReadShort ();

	if (n)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		//		MSG_WriteString (&cls.netchan.message, va("soundlist %i %i", cl.servercount, n));
		MSG_WriteString (&cls.netchan.message, va (soundlist_name, cl.servercount, n));
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_sound;
	Sound_NextDownload ();
}

static void CL_ParseModelList (void)
{
	int nummodels;
	char *str;
	int n;

	// precache models and note certain default indexes
	nummodels = MSG_ReadShort ();

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		nummodels++;
		if (nummodels == MAX_MODELS)
			Host_Error ("Server sent too many model_precache");
		strcpy (cl.model_name[nummodels], str);

		if (!strcmp (cl.model_name[nummodels], "progs/spike.mdl"))
			cl_spikeindex = nummodels;
		if (!strcmp (cl.model_name[nummodels], "progs/player.mdl"))
			cl_playerindex = nummodels;
		if (!strcmp (cl.model_name[nummodels], "progs/flag.mdl"))
			cl_flagindex = nummodels;
	}

	n = MSG_ReadShort ();

	if (n)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		//		MSG_WriteString (&cls.netchan.message, va("modellist %i %i", cl.servercount, n));
		MSG_WriteString (&cls.netchan.message, va (modellist_name, cl.servercount, n));
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_model;
	Model_NextDownload ();
}

static void CL_ParseBaseline (entity_state_t *es)
{
	int i;

	es->modelindex = MSG_ReadShort ();
	es->frame = MSG_ReadByte ();
	es->colormap = MSG_ReadByte ();
	es->skinnum = MSG_ReadByte ();
	for (i = 0; i < 3; i++)
	{
		es->origin[i] = MSG_ReadCoord ();
		es->angles[i] = MSG_ReadAngle ();
	}
}

/*
=====================
CL_ParseStatic

Static entities are non-interactive world objects
like torches
=====================
*/
static void CL_ParseStatic (void)
{
	entity_t *ent;
	int i;
	entity_state_t es;

	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_Error ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;

	CL_ParseBaseline (&es);

	// copy it to the current state
	ent->model = cl.model_precache[es.modelindex];
	ent->cmodel = cl.cmodel_precache[es.modelindex];
	ent->frame = es.frame;
	ent->skinnum = es.skinnum;

	VectorCopy (es.origin, ent->origin);
	VectorCopy (es.angles, ent->angles);

	R_AddEfrags (ent);
}

static void CL_ParseStaticSound (void)
{
	vec3_t org;
	int sound_num, vol, atten;
	int i;

	for (i = 0; i < 3; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadShort ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();

	S_StaticSound (cl.sound_precache[sound_num], org, vol / 255.0f, atten);
}

static void CL_ParseStartSoundPacket (void)
{
	vec3_t pos;
	int channel, ent;
	int sound_num;
	int volume;
	float attenuation;
	int i;

	channel = MSG_ReadShort ();

	if (channel & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	if (channel & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	sound_num = MSG_ReadShort ();

	for (i = 0; i < 3; i++)
		pos[i] = MSG_ReadCoord ();

	ent = (channel >> 3) & 1023;
	channel &= 7;

	if (ent > MIN_EDICTS)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);

	S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume / 255.0f, attenuation);
}

/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
static void CL_ParseClientdata (void)
{
	int i;
	float latency;
	frame_t *frame;

	// calculate simulated time of message

	i = cls.netchan.incoming_acknowledged;
	cl.parsecount = i;
	i &= UPDATE_MASK;
	parsecountmod = i;
	frame = &cl.frames[i];
	parsecounttime = cl.frames[i].senttime;

	frame->receivedtime = host_time;

	// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
		//		Con_Printf ("Odd latency: %5.2f\n", latency);
	}
	else
	{
		// drift the average latency towards the observed latency
		if (latency < cls.latency)
			cls.latency = latency;
		else
			cls.latency += 0.001; // drift up, so correction are needed
	}
}

static void CL_ProcessUserInfo (int slot, player_info_t *player)
{
	strncpy (player->name, Info_ValueForKey (player->userinfo, "name"), sizeof (player->name) - 1);

	if (slot == cl.playernum)
		cl.userid = player->userid;

	Sbar_Changed ();
}

static void CL_UpdateUserinfo (void)
{
	int slot;
	player_info_t *player;

	slot = MSG_ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_Error ("CL_ParseServerMessage: svc_updateuserinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];
	player->userid = MSG_ReadLong ();
	strncpy (player->userinfo, MSG_ReadString (), sizeof (player->userinfo) - 1);

	CL_ProcessUserInfo (slot, player);
}

static void CL_SetInfo (void)
{
	int slot;
	player_info_t *player;
	char key[MAX_MSGLEN];
	char value[MAX_MSGLEN];

	slot = MSG_ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_Error ("CL_ParseServerMessage: svc_setinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];

	strncpy (key, MSG_ReadString (), sizeof (key) - 1);
	key[sizeof (key) - 1] = 0;
	strncpy (value, MSG_ReadString (), sizeof (value) - 1);
	key[sizeof (value) - 1] = 0;

	Con_DPrintf ("SETINFO %s: %s=%s\n", player->name, key, value);

	Info_SetValueForKey (player->userinfo, key, value, MAX_INFO_STRING, true);

	CL_ProcessUserInfo (slot, player);
}

static void CL_ServerInfo (void)
{
	int slot;
	player_info_t *player;
	char key[MAX_MSGLEN];
	char value[MAX_MSGLEN];

	strncpy (key, MSG_ReadString (), sizeof (key) - 1);
	key[sizeof (key) - 1] = 0;
	strncpy (value, MSG_ReadString (), sizeof (value) - 1);
	key[sizeof (value) - 1] = 0;

	Con_DPrintf ("SERVERINFO: %s=%s\n", key, value);

	Info_SetValueForKey (cl.serverinfo, key, value, MAX_SERVERINFO_STRING, true);
}

static void CL_SetStat (int stat, int value)
{
	int j;
	if (stat < 0 || stat >= MAX_CL_STATS)
		Sys_Error ("CL_SetStat: %i is invalid", stat);

	Sbar_Changed ();

	if (stat == STAT_ITEMS)
	{ // set flash times
		Sbar_Changed ();
		for (j = 0; j < 32; j++)
			if ((value & (1 << j)) && !(cl.stats[stat] & (1 << j)))
				cl.item_gettime[j] = cl.time;
	}

	cl.stats[stat] = value;
}

static void CL_MuzzleFlash (void)
{
	vec3_t fv, rv, uv;
	dlight_t *dl;
	int i;
	player_state_t *pl;

	i = MSG_ReadShort ();

	if ((unsigned int)i - 1 >= MAX_CLIENTS)
		return;

	pl = &cl.frames[parsecountmod].playerstate[i - 1];

	dl = CL_AllocDlight (i);
	VectorCopy (pl->origin, dl->origin);
	AngleVectors (pl->viewangles, fv, rv, uv);

	VectorMA (dl->origin, 18, fv, dl->origin);
	dl->radius = 200 + (rand () & 31);
	dl->minlight = 32;
	dl->die = cl.time + 0.1;
	dl->color[0] = 0.2;
	dl->color[1] = 0.1;
	dl->color[2] = 0.05;
	dl->color[3] = 0.7;
}

#define CL_ShowNet(x) if (cl_shownet.value == 2) { Con_Printf ("%3i:%s\n", msg_readcount - 1, x); }

static const char *const svc_nq_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",
	"svc_setview",
	"svc_sound",
	"svc_time",
	"svc_print",
	"svc_stufftext",
	"svc_setangle",
	"svc_serverinfo",
	"svc_lightstyle",
	"svc_updatename",
	"svc_updatefrags",
	"svc_clientdata",
	"svc_stopsound",
	"svc_updatecolors",
	"svc_particle",
	"svc_damage",
	"svc_spawnstatic",
	"svc_21",
	"svc_spawnbaseline",
	"svc_tempentity",
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",
	"svc_cdtrack",
	"svc_sellscreen",
	"svc_cutscene",
};

static const char *const svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_4",
	"svc_setview",
	"svc_sound",
	"svc_12",
	"svc_print",
	"svc_stufftext",
	"svc_setangle",
	"svc_serverdata",
	"svc_lightstyle",
	"svc_13",
	"svc_updatefrags",
	"svc_15",
	"svc_stopsound",
	"svc_17",
	"svc_18",
	"svc_damage",
	"svc_spawnstatic",
	"svc_21",
	"svc_spawnbaseline",
	"svc_tempentity",
	"svc_setpause",
	"svc_25",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",
	"svc_cdtrack",
	"svc_sellscreen",
	"svc_smallkick",
	"svc_bigkick",
	"svc_updateping",
	"svc_updateentertime",
	"svc_updatestatlong",
	"svc_muzzleflash",
	"svc_updateuserinfo",
	"svc_download",
	"svc_playerinfo",
	"svc_nails",
	"svc_chokecount",
	"svc_modellist",
	"svc_soundlist",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_maxspeed",
	"svc_entgravity",
	"svc_setinfo",
	"svc_serverinfo",
	"svc_updateplayer",
};

typedef void (*svc_callback_t) (void);

// TODO: Move these
static int msg_number;
static int msg_protocol;

static void SVC_Deprecated (void)
{
	Host_Error ("CL_ParseServerMessage: Deprecated server message %i for %i", msg_number, msg_protocol);
}
		
static void SVC_Bad (void)
{
	Host_Error ("CL_ParseServerMessage: Illegible server message %i for %i", msg_number, msg_protocol);
}

static void SVC_Nop (void) {}

static void SVC_Disconnect (void)
{
	if (cls.state == ca_connected)
		Host_Error ("Server disconnected\nServer version may not be compatible");
	else
		Host_Error ("Server disconnected");
}

static void SVC_UpdateStat (void)
{
	int i = MSG_ReadByte ();
	int j = MSG_ReadByte ();
	CL_SetStat (i, j);
}

static void SVC_SetView (void)
{
	int i = MSG_ReadByte ();
	int j = MSG_ReadLong ();
	CL_SetStat (i, j);
}

static void SVC_Sound (void)
{
	CL_ParseStartSoundPacket ();
}

static void SVC_Print (void)
{
	int i = MSG_ReadByte ();
	if (i == PRINT_CHAT)
		S_LocalSound (cl_sfx_talk);
	Con_Printf ("%s", MSG_ReadString ());
}

static void SVC_StuffText (void)
{
	char *s = MSG_ReadString ();
	Con_DPrintf ("stufftext: %s\n", s);
	Cbuf_AddText (src_client, s);
}

static void SVC_SetAngle (void)
{
	for (int i = 0; i < 3; i++)
		cl.viewangles[i] = MSG_ReadAngle ();
}

static void SVC_ServerData (void)
{
	Cbuf_Execute (src_client); // make sure any stuffed commands are done
	CL_ParseServerData ();
	vid.recalc_refdef = true; // leave full screen intermission
}

static void SVC_LightStyle (void)
{
	int i = MSG_ReadByte ();
	if (i >= MAX_LIGHTSTYLES)
		Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
	strcpy (cl_lightstyle[i].map, MSG_ReadString ());
	cl_lightstyle[i].length = strlen (cl_lightstyle[i].map);
}

static void SVC_UpdateFrags (void)
{
	Sbar_Changed ();
	int i = MSG_ReadByte ();
	if (i >= MAX_CLIENTS)
		Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
	cl.players[i].frags = MSG_ReadShort ();
}

static void SVC_StopSound (void)
{
	int i = MSG_ReadShort ();
	S_StopSound (i >> 3, i & 7);
}

static void SVC_Particle (void)
{
	R_ParseParticleEffect ();
}

static void SVC_Damage (void)
{
	V_ParseDamage ();
}

static void SVC_SpawnStatic (void)
{
	CL_ParseStatic ();
}

static void SVC_SpawnBaseline (void)
{
	int i = MSG_ReadShort ();
	CL_ParseBaseline (&cl_baselines[i]);
}

static void SVC_TempEntity (void)
{
	CL_ParseTEnt (msg_protocol == PROTOCOL_QUAKEWORLD);
}

static void SVC_SetPause (void)
{
	const bool paused = MSG_ReadByte ();
	if (cl.paused != paused)
	{
		cl.paused = paused;
		S_SetSoundPaused (cl.paused);
	}
}

static void SVC_CenterPrint (void)
{
	SCR_CenterPrint (MSG_ReadString ());
}

static void SVC_KilledMonster (void)
{
	cl.stats[STAT_MONSTERS]++;
}

static void SVC_FoundSecret (void)
{
	cl.stats[STAT_SECRETS]++;
}

static void SVC_SpawnStaticSound (void)
{
	CL_ParseStaticSound ();
}

static void SVC_Intermission (void)
{
	cl.intermission = 1;
	cl.completed_time = cl.time;
	vid.recalc_refdef = true; // go to full screen
	if (msg_protocol == PROTOCOL_QUAKEWORLD)
	{
		for (int i = 0; i < 3; i++)
			cl.simorg[i] = MSG_ReadCoord ();
		for (int i = 0; i < 3; i++)
			cl.simangles[i] = MSG_ReadAngle ();
		VectorCopy (vec3_origin, cl.simvel);
	}
}

static void SVC_Finale (void)
{
	cl.intermission = 2;
	cl.completed_time = cl.time;
	vid.recalc_refdef = true; // go to full screen
	SCR_CenterPrint (MSG_ReadString ());
}

static void SVC_CDTrack (void)
{
	cl.cdtrack = MSG_ReadByte ();
	bool looping = true;
	if (msg_protocol != PROTOCOL_QUAKEWORLD)
		looping = MSG_ReadByte ();
	Music_Play (cl.cdtrack, looping);
}

static void SVC_SellScreen (void)
{
	Cmd_ExecuteString (src_client, "help");
}

static void SVC_Cutscene (void)
{
	cl.intermission = 3;
	cl.completed_time = cl.time;
	vid.recalc_refdef = true; // go to full screen
	SCR_CenterPrint (MSG_ReadString ());
}

static void SVC_SmallKick (void)
{
	cl.punchangle = -2;
}

static void SVC_BigKick (void)
{
	cl.punchangle = -4;
}

static void SVC_UpdatePing (void)
{
	int i = MSG_ReadByte ();
	if (i >= MAX_CLIENTS)
		Host_Error ("CL_ParseServerMessage: svc_updateping > MAX_SCOREBOARD");
	cl.players[i].ping = MSG_ReadShort ();
}

static void SVC_UpdateEnterTime (void)
{
	// time is sent over as seconds ago
	int i = MSG_ReadByte ();
	if (i >= MAX_CLIENTS)
		Host_Error ("CL_ParseServerMessage: svc_updateentertime > MAX_SCOREBOARD");
	cl.players[i].entertime = host_time - MSG_ReadFloat ();
}

static void SVC_UpdateStatLong (void)
{
	int i = MSG_ReadByte ();
	int j = MSG_ReadLong ();
	CL_SetStat (i, j);
}

static void SVC_MuzzleFlash (void)
{
	CL_MuzzleFlash ();
}

static void SVC_UpdateUserInfo (void)
{
	CL_UpdateUserinfo ();
}

static void SVC_Download (void)
{
	CL_ParseDownload ();
}

static void SVC_PlayerInfo (void)
{
	CL_ParsePlayerinfo ();
}

static void SVC_Nails (void)
{
	CL_ParseProjectiles ();
}

static void SVC_ChokeCount (void)
{
	int i = MSG_ReadByte ();
	for (int j = 0; j < i; j++)
		cl.frames[(cls.netchan.incoming_acknowledged - 1 - j) & UPDATE_MASK].receivedtime = -2;
}

static void SVC_ModelList (void)
{
	CL_ParseModelList ();
}

static void SVC_SoundList (void)
{
	CL_ParseSoundList ();
}

static void SVC_PacketEntities (void)
{
	CL_ParsePacketEntities (false);
}

static void SVC_DeltaPacketEntities (void)
{
	CL_ParsePacketEntities (true);
}

static void SVC_MaxSpeed (void)
{
	movevars.maxspeed = MSG_ReadFloat ();
}

static void SVC_EntGravity (void)
{
	movevars.entgravity = MSG_ReadFloat ();
}

static void SVC_SetInfo (void)
{
	CL_SetInfo ();
}

static void SVC_ServerInfo (void)
{
	CL_ServerInfo ();
}

static void SVC_UpdatePlayer (void)
{
	int i = MSG_ReadByte ();
	if (i >= MAX_CLIENTS)
		Host_Error ("CL_ParseServerMessage: svc_updatepl > MAX_SCOREBOARD");
	cl.players[i].pl = MSG_ReadByte ();
}

static const svc_callback_t svc_nq_callbacks[] = {
	SVC_Bad,
	SVC_Nop,
	SVC_Disconnect,
	SVC_UpdateStat,
	SVC_Deprecated, // SVC_Version
	SVC_SetView,
	SVC_Sound,
	SVC_Deprecated, // SVC_Time
	SVC_Print,
	SVC_StuffText,
	SVC_SetAngle,
	SVC_ServerInfo,
	SVC_LightStyle,
	SVC_Deprecated, // SVC_UpdateName
	SVC_UpdateFrags,
	SVC_Deprecated, // SVC_ClientData
	SVC_StopSound,
	SVC_Deprecated, // SVC_UpdateColors
	SVC_Particle,
	SVC_Damage,
	SVC_SpawnStatic,
	SVC_Deprecated,
	SVC_SpawnBaseline,
	SVC_TempEntity,
	SVC_SetPause,
	SVC_Deprecated, // SVC_SignonNum
	SVC_CenterPrint,
	SVC_KilledMonster,
	SVC_FoundSecret,
	SVC_SpawnStaticSound,
	SVC_Intermission,
	SVC_Finale,
	SVC_CDTrack,
	SVC_SellScreen,
	SVC_Cutscene,
};

static void SVC_NQ_ParseServerMessage (void)
{
	if (msg_number <= -1 || msg_number >= lengthof (svc_nq_callbacks))
	{
		SVC_Bad ();
		return;
	}
	CL_ShowNet (svc_nq_strings[msg_number]);
	svc_nq_callbacks[msg_number]();
}

static const svc_callback_t svc_callbacks[] = {
	SVC_Bad,
	SVC_Nop,
	SVC_Disconnect,
	SVC_UpdateStat,
	SVC_Deprecated, // SVC_Version
	SVC_SetView,
	SVC_Sound,
	SVC_Deprecated, // SVC_Time
	SVC_Print,
	SVC_StuffText,
	SVC_SetAngle,
	SVC_ServerData,
	SVC_LightStyle,
	SVC_Deprecated, // SVC_UpdateName
	SVC_UpdateFrags,
	SVC_Deprecated, // SVC_ClientData
	SVC_StopSound,
	SVC_Deprecated, // SVC_UpdateColors
	SVC_Particle,
	SVC_Damage,
	SVC_SpawnStatic,
	SVC_Deprecated,
	SVC_SpawnBaseline,
	SVC_TempEntity,
	SVC_SetPause,
	SVC_Deprecated, // SVC_SignonNum
	SVC_CenterPrint,
	SVC_KilledMonster,
	SVC_FoundSecret,
	SVC_SpawnStaticSound,
	SVC_Intermission,
	SVC_Finale,
	SVC_CDTrack,
	SVC_SellScreen,
	SVC_SmallKick,
	SVC_BigKick,
	SVC_UpdatePing,
	SVC_UpdateEnterTime,
	SVC_UpdateStatLong,
	SVC_MuzzleFlash,
	SVC_UpdateUserInfo,
	SVC_Download,
	SVC_PlayerInfo,
	SVC_Nails,
	SVC_ChokeCount,
	SVC_ModelList,
	SVC_SoundList,
	SVC_PacketEntities,
	SVC_DeltaPacketEntities,
	SVC_MaxSpeed,
	SVC_EntGravity,
	SVC_SetInfo,
	SVC_ServerInfo,
	SVC_UpdatePlayer,
};

static void SVC_ParseServerMessage (void)
{
	if (msg_number <= -1 || msg_number >= lengthof (svc_callbacks))
	{
		SVC_Bad ();
		return;
	}
	CL_ShowNet (svc_strings[msg_number]);
	svc_callbacks[msg_number]();
}

void CL_ParseServerMessage (void)
{
	char *s;
	int i, j;

	cl.last_servermessage = host_time;
	CL_ClearProjectiles ();

	//
	// if recording demos, copy the message out
	//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ", net_message[CLIENT].cursize);
	else if (cl_shownet.value == 2)
		Con_Printf ("------------------\n");

	CL_ParseClientdata ();

	//
	// parse the message
	//
	while (1)
	{
		if (msg_badread)
		{
			Host_Error ("CL_ParseServerMessage: Bad server message");
			break;
		}

		msg_number = MSG_ReadByte ();

		if (msg_number == -1)
		{
			msg_readcount++; // so the EOM showner has the right value
			CL_ShowNet ("END OF MESSAGE");
			break;
		}

		// Messages from the engine are always in QW format
		if (msg_number > svc_reserved)
		{
			msg_number -= svc_reserved + 1;
			msg_protocol = PROTOCOL_QUAKEWORLD;
		}
		else
			msg_protocol = cls.protocol;

		switch (msg_protocol)
		{
		case PROTOCOL_NETQUAKE:
		{
			SVC_NQ_ParseServerMessage ();
			break;
		}
		case PROTOCOL_QUAKEWORLD:
		{
			SVC_ParseServerMessage ();
			break;
		}
		}
	}

	CL_SetSolidEntities ();
}
