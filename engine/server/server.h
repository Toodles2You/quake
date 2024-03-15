
#ifndef _SERVER_H
#define _SERVER_H

#define NUM_SPAWN_PARMS 16

typedef enum
{
	cs_free,	  // can be reused for a new connection
	cs_zombie,	  // client has been disconnected, but don't reuse
				  // connection for a couple seconds
	cs_connected, // has been assigned to a client_t, but not in game yet
	cs_spawned	  // client is fully in game
} client_state_e;

typedef struct
{
	// received from client

	// reply
	double senttime;
	float ping_time;
	packet_entities_t entities;
} client_frame_t;

#define MAX_BACK_BUFFERS 4

typedef struct client_s
{
	client_state_e state;

	int spectator; // non-interactive

	bool sendinfo;		// at end of frame, send info to all
						// this prevents malicious multiple broadcasts
	float lastnametime; // time of last name change
	int lastnamecount;	// time of last name change
	unsigned checksum;	// checksum for calcs
	bool drop;			// lose this guy next opportunity
	int lossage;		// loss percentage

	int userid;						// identifying number
	char userinfo[MAX_INFO_STRING]; // infostring

	usercmd_t lastcmd; // for filling in big drops and partial predictions
	double localtime;  // of last message
	int oldbuttons;

	float maxspeed;	  // localized maxspeed
	float entgravity; // localized ent gravity

	edict_t *edict;	  // EDICT_NUM(clientnum+1)
	char name[32];	  // for printing to other people
					  // extracted from userinfo
	int messagelevel; // for filtering printed messages

	// the datagram is written to after every frame, but only cleared
	// when it is sent out to the client.  overflow is tolerated.
	sizebuf_t datagram;
	byte datagram_buf[MAX_DATAGRAM];

	// back buffers for client reliable data
	sizebuf_t backbuf;
	int num_backbuf;
	int backbuf_size[MAX_BACK_BUFFERS];
	byte backbuf_data[MAX_BACK_BUFFERS][MAX_MSGLEN];

	double connection_started; // or time of disconnect for zombies
	bool send_message;		   // set on frames a datagram arived on

	// spawn parms are carried from level to level
	float spawn_parms[NUM_SPAWN_PARMS];

	// client known data for deltas
	int old_frags;

	int stats[MAX_CL_STATS];

	client_frame_t frames[UPDATE_BACKUP]; // updates can be deltad from here

	FILE *download;	   // file being downloaded
	int downloadsize;  // total bytes
	int downloadcount; // bytes sent

	int spec_track; // entnum of player tracking

	double whensaid[10]; // JACK: For floodprots
	int whensaidhead;	 // Head value for floodprots
	double lockedtill;

	FILE *upload;
	char uploadfn[MAX_QPATH];
	netadr_t snap_from;
	bool remote_snap;

	//===== NETWORK ============
	int chokecount;
	int delta_sequence; // -1 = no compression
	netchan_t netchan;
} client_t;


#define STATFRAMES 100
typedef struct
{
	double active;
	double idle;
	int count;
	int packets;

	double latched_active;
	double latched_idle;
	int latched_packets;
} svstats_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES 1024

typedef struct
{
	netadr_t adr;
	int challenge;
	int time;
} challenge_t;

typedef struct
{
	int protocol;
	int spawncount; // number of servers spawned since start,
					// used to check late spawns
	client_t clients[MAX_CLIENTS];
	int serverflags; // episode completion information

	double last_heartbeat;
	int heartbeat_sequence;
	svstats_t stats;

	char info[MAX_SERVERINFO_STRING];

	// log messages are used so that fraglog processes can get stats
	int logsequence; // the message currently being filled
	double logtime;	 // time of last swap
	sizebuf_t log[2];
	byte log_buf[2][MAX_DATAGRAM];

	challenge_t challenges[MAX_CHALLENGES]; // to prevent invalid IPs from connecting
} server_static_t;

//=============================================================================

typedef enum
{
	ss_dead,	// no map loaded
	ss_loading, // spawning level edicts
	ss_active,	// actively running
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

#define MAX_MASTERS 8 // max recipients for heartbeat packets

#define MAX_SIGNON_BUFFERS 8

typedef struct
{
	bool active;		  // false when server is going down
	server_state_t state; // precache commands are only valid during load

	double time;
	double oldtime;
	double frametime;

	int lastcheck;		  // used by PF_checkclient
	double lastchecktime; // for monster ai

	bool paused; // are we paused?

	// check player/eyes models for hacks
	unsigned model_player_checksum;
	unsigned eyes_player_checksum;

	char name[64];			   // map name
	char startspot[64];
	char modelname[MAX_QPATH]; // maps/<name>.bsp, for model_precache[0]
	struct cmodel_s *worldmodel;
	char *model_precache[MAX_MODELS]; // NULL terminated
	char *sound_precache[MAX_SOUNDS]; // NULL terminated
	char *lightstyles[MAX_LIGHTSTYLES];
	struct cmodel_s *models[MAX_MODELS];

	size_t num_edicts;
	size_t max_edicts;
	edict_t *edicts; // can NOT be array indexed, because
					 // edict_t is variable sized, but can
					 // be used to reference the world ent

	progs_state_t pr;

	byte *pvs, *phs; // fully expanded and decompressed

	// added to every client's unreliable buffer each frame, then cleared
	sizebuf_t datagram;
	byte datagram_buf[MAX_DATAGRAM];

	// added to every client's reliable buffer each frame, then cleared
	sizebuf_t reliable_datagram;
	byte reliable_datagram_buf[MAX_MSGLEN];

	// the multicast buffer is used to send a message to a set of clients
	sizebuf_t multicast;
	byte multicast_buf[MAX_MSGLEN];

	// the master buffer is used for building log packets
	sizebuf_t master;
	byte master_buf[MAX_DATAGRAM];

	// the signon buffer will be sent to each client as they connect
	// includes the entity baselines, the static entities, etc
	// large levels will have >MAX_DATAGRAM sized signons, so
	// multiple signon messages are kept
	sizebuf_t signon;
	int num_signon_buffers;
	int signon_buffer_size[MAX_SIGNON_BUFFERS];
	byte signon_buffers[MAX_SIGNON_BUFFERS][MAX_DATAGRAM];
} server_t;

//=============================================================================

// edict->deadflag values
enum
{
	DEAD_NO = 0,
	DEAD_DYING,
	DEAD_DEAD,
};

// edict->takedamage values
enum
{
	DAMAGE_NO = 0,
	DAMAGE_YES,
	DAMAGE_AIM,
};

// edict->flags values
enum
{
	FL_FLY				= 1 << 0,
	FL_SWIM				= 1 << 1,
	FL_CONVEYOR			= 1 << 2,
	FL_CLIENT			= 1 << 3,
	FL_INWATER			= 1 << 4,
	FL_MONSTER			= 1 << 5,
	FL_GODMODE			= 1 << 6,
	FL_NOTARGET			= 1 << 7,
	FL_ITEM				= 1 << 8,
	FL_ONGROUND			= 1 << 9,
	FL_PARTIALGROUND	= 1 << 10, // not all corners are valid
	FL_WATERJUMP		= 1 << 11, // player jumping out of water
	FL_JUMPRELEASED		= 1 << 12, // for jump debouncing
	FL_ARCHIVE_OVERRIDE = 1048576,
};

// spawn flags
enum
{
	SPAWNFLAG_NOT_EASY			= 1 << 8,
	SPAWNFLAG_NOT_MEDIUM		= 1 << 9,
	SPAWNFLAG_NOT_HARD			= 1 << 10,
	SPAWNFLAG_NOT_DEATHMATCH	= 1 << 11,
};

enum
{
	MULTICAST_ALL = 0,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R,
};

// server flags
enum
{
	SFL_EPISODE_1		= 1 << 0,
	SFL_EPISODE_2		= 1 << 1,
	SFL_EPISODE_3		= 1 << 2,
	SFL_EPISODE_4		= 1 << 3,
	SFL_NEW_UNIT		= 1 << 4,
	SFL_NEW_EPISODE		= 1 << 5,
	SFL_CROSS_TRIGGERS	= 65280,
};

//============================================================================

extern	cvar_t	sv_maxspeed;

extern cvar_t teamplay;
extern cvar_t skill;
extern cvar_t deathmatch;
extern cvar_t coop;
extern cvar_t fraglimit;
extern cvar_t timelimit;

extern cvar_t sv_highchars;

extern server_static_t svs; // persistant server info
extern server_t sv;			// local server

extern client_t *host_client;

extern jmp_buf host_abortserver;

extern double host_time;

extern edict_t *sv_player;

extern char localmodels[MAX_MODELS][5]; // inline model names for precache

extern char localinfo[MAX_LOCALINFO_STRING + 1];

extern FILE *sv_fraglogfile;

//===========================================================

//
// sv_main.c
//
void SV_Shutdown();
void SV_Frame(float time);
void SV_FinalMessage(char *message);
void SV_DropClient(client_t *drop);

int SV_CalcPing(client_t *cl);
void SV_FullClientUpdate(client_t *client, sizebuf_t *buf);

int SV_ModelIndex(char *name);

bool SV_CheckBottom(edict_t *ent);
bool SV_movestep(edict_t *ent, vec3_t move, bool relink);

void SV_MoveToGoal();

void SV_SaveSpawnparms();

void SV_Physics_Client(edict_t *ent);

void SV_ExecuteUserCommand(char *s);
void SV_InitOperatorCommands();

void SV_SendServerinfo(client_t *client);
void SV_ExtractFromUserinfo(client_t *cl);

void Master_Heartbeat();
void Master_Packet();
void Master_Shutdown ();

//
// sv_init.c
//
void SV_SpawnServer(char *server, char *startspot);
void SV_FlushSignon();

//
// sv_phys.c
//
void SV_ProgStartFrame();
void SV_Physics();
void SV_CheckVelocity(edict_t *ent);
void SV_AddGravity(edict_t *ent, float scale);
bool SV_RunThink(edict_t *ent);
void SV_Physics_Toss(edict_t *ent);
void SV_RunNewmis();
void SV_Impact(edict_t *e1, edict_t *e2);
void SV_SetMoveVars();

//
// sv_send.c
//
void SV_SendClientMessages();

void SV_Multicast(vec3_t origin, int to);
void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound(edict_t *entity, int channel, char *sample, int volume, float attenuation);
void SV_ClientPrintf(client_t *cl, int level, char *fmt, ...);
void SV_BroadcastPrintf(int level, char *fmt, ...);
void SV_BroadcastCommand(char *fmt, ...);
void SV_SendMessagesToAll();
void SV_FindModelNumbers();

void SV_BeginRedirect (redirect_t rd);
void SV_EndRedirect ();

//
// sv_user.c
//
void SV_ExecuteClientMessage(client_t *cl);
void SV_UserInit();
void SV_TogglePause(const char *msg);

//
// sv_ccmds.c
//
void SV_Status_f();

//
// sv_ents.c
//
void SV_WriteEntitiesToClient(client_t *client, sizebuf_t *msg);

//
// sv_nchan.c
//
void ClientReliableCheckBlock(client_t *cl, int maxsize);
void ClientReliable_FinishWrite(client_t *cl);
void ClientReliableWrite_Begin(client_t *cl, int c, int maxsize);
void ClientReliableWrite_Angle(client_t *cl, float f);
void ClientReliableWrite_Angle16(client_t *cl, float f);
void ClientReliableWrite_Byte(client_t *cl, int c);
void ClientReliableWrite_Char(client_t *cl, int c);
void ClientReliableWrite_Float(client_t *cl, float f);
void ClientReliableWrite_Coord(client_t *cl, float f);
void ClientReliableWrite_Long(client_t *cl, int c);
void ClientReliableWrite_Short(client_t *cl, int c);
void ClientReliableWrite_String(client_t *cl, char *s);
void ClientReliableWrite_SZ(client_t *cl, void *data, int len);

#endif /* !_SERVER_H */
