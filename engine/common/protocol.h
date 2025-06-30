
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

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
	PF_NOGRAV = 1 << 11,	 // don't apply gravity for prediction

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
	svc_bad = 0,
	svc_nop = 1,
	svc_disconnect = 2,
	svc_updatestat = 3, // [byte] [byte]
	svc_setview = 5,	// [short] entity number
	svc_sound = 6,		// <see code>
	svc_print = 8,		// [byte] id [string] null terminated string
	svc_stufftext = 9,	// [string] stuffed into client's console buffer
						// the string should be \n terminated
	svc_setangle = 10,	// [angle3] set the view angle to this absolute value

	svc_serverdata = 11,  // [long] protocol ...
	svc_lightstyle = 12,  // [byte] [string]
	svc_updatefrags = 14, // [byte] [short]
	svc_stopsound = 16,	  // <see code>
	svc_particle = 18,	  // [vec3] <variable>
	svc_damage = 19,

	svc_spawnstatic = 20,
	svc_spawnbaseline = 22,

	svc_temp_entity = 23, // variable
	svc_setpause = 24,	  // [byte] on / off

	svc_centerprint = 26, // [string] to put in center of the screen

	svc_killedmonster = 27,
	svc_foundsecret = 28,

	svc_spawnstaticsound = 29, // [coord3] [byte] samp [byte] vol [byte] aten

	svc_intermission = 30, // [vec3_t] origin [vec3_t] angle
	svc_finale = 31,	   // [string] text

	svc_cdtrack = 32, // [byte] track
	svc_sellscreen = 33,

	svc_smallkick = 34, // set client punchangle to 2
	svc_bigkick = 35,	// set client punchangle to 4

	svc_updateping = 36,	  // [byte] [short]
	svc_updateentertime = 37, // [byte] [float]

	svc_updatestatlong = 38, // [byte] [long]

	svc_muzzleflash = 39, // [short] entity

	svc_updateuserinfo = 40, // [byte] slot [long] uid
							 // [string] userinfo

	svc_download = 41,			  // [short] size [size bytes]
	svc_playerinfo = 42,		  // variable
	svc_nails = 43,				  // [byte] num [48 bits] xyzpy 12 12 12 4 8
	svc_chokecount = 44,		  // [byte] packets choked
	svc_modellist = 45,			  // [strings]
	svc_soundlist = 46,			  // [strings]
	svc_packetentities = 47,	  // [...]
	svc_deltapacketentities = 48, // [...]
	svc_maxspeed = 49,			  // maxspeed change, for prediction
	svc_entgravity = 50,		  // gravity change, for prediction
	svc_setinfo = 51,			  // setinfo on a client
	svc_serverinfo = 52,		  // serverinfo
	svc_updatepl = 53,			  // [byte] [byte]
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

#define UPDATE_BACKUP                                                                                                                                          \
	64	// copies of entity_state_t to keep buffered                                                                                                           \
		// must be power of two
#define UPDATE_MASK (UPDATE_BACKUP - 1)

// entity_state_t is the information conveyed from the server
// in an update message
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
