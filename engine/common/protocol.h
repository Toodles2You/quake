
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

// engine protocol
#define PROTOCOL_VERSION 451

// game protocols (because someone thought it was a good idea to let progs send packets)
enum
{
	PROTOCOL_NETQUAKE = 15,
	PROTOCOL_QUAKEWORLD = 28,
};

enum
{
	PORT_CLIENT = 27001,
	PORT_MASTER = 27000,
	PORT_SERVER = 27500,
};

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)
enum
{
	S2C_CHALLENGE = 'c',
	S2C_CONNECTION = 'j',
	A2A_PING = 'k',	 // respond with an A2A_ACK
	A2A_ACK = 'l',	 // general acknowledgement without info
	A2A_NACK = 'm',	 // [+ comment] general failure
	A2A_ECHO = 'e',	 // for echoing
	A2C_PRINT = 'n', // print a message on client

	S2M_HEARTBEAT = 'a',	  // + serverinfo + userlist + fraglist
	A2C_CLIENT_COMMAND = 'B', // + command line
	S2M_SHUTDOWN = 'C',
};

enum
{
	// playerinfo flags from server
	// playerinfo allways sends: playernum, flags, origin[] and framenumber
	PF_MSEC = 1 << 0,
	PF_COMMAND = 1 << 1,
	PF_VELOCITY1 = 1 << 2,
	PF_VELOCITY2 = 1 << 3,
	PF_VELOCITY3 = 1 << 4,
	PF_MODEL = 1 << 5,
	PF_SKINNUM = 1 << 6,
	PF_EFFECTS = 1 << 7,
	PF_WEAPONFRAME = 1 << 8, // only sent for view player
	PF_DEAD = 1 << 9,		 // don't block movement any more
	PF_GIB = 1 << 10,		 // offset the view height differently
	PF_ONGROUND = 1 << 11,	 // don't apply gravity for prediction

	// if the high bit of the client to server byte is set, the low bits are
	// client move cmd bits
	// ms and angle2 are allways sent, the others are optional
	CM_ANGLE1 = 1 << 0,
	CM_ANGLE3 = 1 << 1,
	CM_FORWARD = 1 << 2,
	CM_SIDE = 1 << 3,
	CM_UP = 1 << 4,
	CM_BUTTONS = 1 << 5,
	CM_IMPULSE = 1 << 6,
	CM_ANGLE2 = 1 << 7,

	// the first 16 bits of a packetentities update holds 9 bits
	// of entity number and 7 bits of flags
	U_ORIGIN1 = 1 << 9,
	U_ORIGIN2 = 1 << 10,
	U_ORIGIN3 = 1 << 11,
	U_ANGLE2 = 1 << 12,
	U_FRAME = 1 << 13,
	U_REMOVE = 1 << 14, // REMOVE this entity, don't add it
	U_MOREBITS = 1 << 15,

	// if MOREBITS is set, these additional flags are read in next
	U_ANGLE1 = 1 << 0,
	U_ANGLE3 = 1 << 1,
	U_MODEL = 1 << 2,
	U_COLORMAP = 1 << 3,
	U_SKIN = 1 << 4,
	U_EFFECTS = 1 << 5,
	U_SOLID = 1 << 6, // the entity should be solid for prediction
};

// a sound with no channel is a local only sound
enum
{
	SND_VOLUME = 1 << 15,	   // a byte
	SND_ATTENUATION = 1 << 14, // a byte
};

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
enum
{
	svc_nq_bad,
	svc_nq_nop,
	svc_nq_disconnect,
	svc_nq_updatestat,
	svc_nq_version,
	svc_nq_setview,
	svc_nq_sound,
	svc_nq_time,
	svc_nq_print,
	svc_nq_stufftext,
	svc_nq_setangle,
	svc_nq_serverinfo,
	svc_nq_lightstyle,
	svc_nq_updatename,
	svc_nq_updatefrags,
	svc_nq_clientdata,
	svc_nq_stopsound,
	svc_nq_updatecolors,
	svc_nq_particle,
	svc_nq_damage,
	svc_nq_spawnstatic,
	svc_nq_21,
	svc_nq_spawnbaseline,
	svc_nq_tempentity,
	svc_nq_setpause,
	svc_nq_signonnum,
	svc_nq_centerprint,
	svc_nq_killedmonster,
	svc_nq_foundsecret,
	svc_nq_spawnstaticsound,
	svc_nq_intermission,
	svc_nq_finale,
	svc_nq_cdtrack,
	svc_nq_sellscreen,
	svc_nq_cutscene,
};

enum
{
	svc_qw_bad,
	svc_qw_nop,
	svc_qw_disconnect,
	svc_qw_updatestat,
	svc_qw_4,
	svc_qw_setview,
	svc_qw_sound,
	svc_qw_12,
	svc_qw_print,
	svc_qw_stufftext,
	svc_qw_setangle,
	svc_qw_serverdata,
	svc_qw_lightstyle,
	svc_qw_13,
	svc_qw_updatefrags,
	svc_qw_15,
	svc_qw_stopsound,
	svc_qw_17,
	svc_qw_18,
	svc_qw_damage,
	svc_qw_spawnstatic,
	svc_qw_21,
	svc_qw_spawnbaseline,
	svc_qw_tempentity,
	svc_qw_setpause,
	svc_qw_25,
	svc_qw_centerprint,
	svc_qw_killedmonster,
	svc_qw_foundsecret,
	svc_qw_spawnstaticsound,
	svc_qw_intermission,
	svc_qw_finale,
	svc_qw_cdtrack,
	svc_qw_sellscreen,
	svc_qw_smallkick,
	svc_qw_bigkick,
	svc_qw_updateping,
	svc_qw_updateentertime,
	svc_qw_updatestatlong,
	svc_qw_muzzleflash,
	svc_qw_updateuserinfo,
	svc_qw_download,
	svc_qw_playerinfo,
	svc_qw_nails,
	svc_qw_chokecount,
	svc_qw_modellist,
	svc_qw_soundlist,
	svc_qw_packetentities,
	svc_qw_deltapacketentities,
	svc_qw_maxspeed,
	svc_qw_entgravity,
	svc_qw_setinfo,
	svc_qw_serverinfo,
	svc_qw_updateplayer,
};

enum
{
	svc_reserved = 63,
	svc_bad,
	svc_nop,
	svc_disconnect,
	svc_updatestat,
	svc_version,
	svc_setview,
	svc_sound,
	svc_time,
	svc_print,
	svc_stufftext,
	svc_setangle,
	svc_serverdata,
	svc_lightstyle,
	svc_updatename,
	svc_updatefrags,
	svc_clientdata,
	svc_stopsound,
	svc_updatecolors,
	svc_particle,
	svc_damage,
	svc_spawnstatic,
	svc_21,
	svc_spawnbaseline,
	svc_tempentity,
	svc_setpause,
	svc_signonnum,
	svc_centerprint,
	svc_killedmonster,
	svc_foundsecret,
	svc_spawnstaticsound,
	svc_intermission,
	svc_finale,
	svc_cdtrack,
	svc_sellscreen,
	svc_smallkick,
	svc_bigkick,
	svc_updateping,
	svc_updateentertime,
	svc_updatestatlong,
	svc_muzzleflash,
	svc_updateuserinfo,
	svc_download,
	svc_playerinfo,
	svc_nails,
	svc_chokecount,
	svc_modellist,
	svc_soundlist,
	svc_packetentities,
	svc_deltapacketentities,
	svc_maxspeed,
	svc_entgravity,
	svc_setinfo,
	svc_serverinfo,
	svc_updateplayer,
};

//
// client to server
//
enum
{
	clc_bad = 0,
	clc_nop = 1,
	clc_move = 3,	   // [[usercmd_t]
	clc_stringcmd = 4, // [string] message
	clc_delta = 5,	   // [byte] sequence number, requests delta compression of message
	clc_upload = 7,
};

//
// temp entity events
//
enum
{
	TE_SPIKE = 0,
	TE_SUPERSPIKE,
	TE_GUNSHOT,
	TE_EXPLOSION,
	TE_TAREXPLOSION,
	TE_LIGHTNING1,
	TE_LIGHTNING2,
	TE_WIZSPIKE,
	TE_KNIGHTSPIKE,
	TE_LIGHTNING3,
	TE_LAVASPLASH,
	TE_TELEPORT,
	TE_BLOOD,
	TE_LIGHTNINGBLOOD,
};

#define MAX_CLIENTS 32

#define UPDATE_BACKUP 64 // copies of entity_state_t to keep buffered; must be power of two
#define UPDATE_MASK (UPDATE_BACKUP - 1)

// entity_state_t is the information conveyed from the server in an update message
typedef struct
{
	int number; // edict index

	int flags; // nolerp, etc
	vec3_t origin;
	vec3_t angles;
	int modelindex;
	int frame;
	int colormap;
	int skinnum;
	int effects;
} entity_state_t;

#define MAX_PACKET_ENTITIES 64 // doesn't count nails
typedef struct
{
	int num_entities;
	entity_state_t entities[MAX_PACKET_ENTITIES];
} packet_entities_t;

typedef struct usercmd_s
{
	byte msec;
	vec3_t angles;
	short forwardmove, sidemove, upmove;
	byte buttons;
	byte impulse;
} usercmd_t;

#endif /* !_PROTOCOL_H */
