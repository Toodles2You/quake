
#ifndef _SERVER_H
#define _SERVER_H

typedef struct
{
	int maxclients;
	int maxclientslimit;
	struct client_s *clients; // [maxclients]
	int serverflags;		  // episode completion information
	bool changelevel_issued;  // cleared when at SV_SpawnServer
} server_static_t;

//=============================================================================

typedef enum
{
	ss_loading,
	ss_active
} server_state_t;

typedef struct
{
	bool active; // false if only a net client

	bool paused;
	bool loadgame; // handle connections specially

	double time;

	int lastcheck; // used by PF_checkclient
	double lastchecktime;

	char name[64]; // map name
	char startspot[64];
	char modelname[64]; // maps/<name>.bsp, for model_precache[0]
	struct cmodel_s *worldmodel;
	char *model_precache[MAX_MODELS]; // NULL terminated
	struct cmodel_s *models[MAX_MODELS];
	char *sound_precache[MAX_SOUNDS]; // NULL terminated
	char *lightstyles[MAX_LIGHTSTYLES];
	int num_edicts;
	int max_edicts;
	edict_t *edicts;	  // can NOT be array indexed, because
						  // edict_t is variable sized, but can
						  // be used to reference the world ent
	server_state_t state; // some actions are only valid during load

	sizebuf_t datagram;
	byte datagram_buf[MAX_DATAGRAM];

	sizebuf_t reliable_datagram; // copied to all clients at end of frame
	byte reliable_datagram_buf[MAX_DATAGRAM];

	sizebuf_t signon;
	byte signon_buf[8192];
} server_t;

#define NUM_PING_TIMES 16
#define NUM_SPAWN_PARMS 16

typedef struct client_s
{
	bool active;	 // false = client is free
	bool spawned;	 // false = don't send datagrams
	bool dropasap;	 // has been told to go to another level
	bool privileged; // can execute any host command
	bool sendsignon; // only valid before spawned

	double last_message; // reliable messages must be sent
						 // periodically

	struct qsocket_s *netconnection; // communications handle

	usercmd_t cmd;	// movement
	vec3_t wishdir; // intended motion calced from cmd

	sizebuf_t message; // can be added to at any time,
					   // copied and clear once per frame
	byte msgbuf[MAX_MSGLEN];
	edict_t *edict; // EDICT_NUM(clientnum+1)
	char name[32];	// for printing to other people
	int colors;

	float ping_times[NUM_PING_TIMES];
	int num_pings; // ping_times[num_pings%NUM_PING_TIMES]

	// spawn parms are carried from level to level
	float spawn_parms[NUM_SPAWN_PARMS];

	// client known data for deltas
	int old_frags;
} client_t;

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

extern cvar_t teamplay;
extern cvar_t skill;
extern cvar_t deathmatch;
extern cvar_t coop;
extern cvar_t fraglimit;
extern cvar_t timelimit;

extern server_static_t svs; // persistant server info
extern server_t sv;			// local server

extern client_t *host_client;

extern jmp_buf host_abortserver;

extern double host_time;

extern edict_t *sv_player;

//===========================================================

void SV_Init();

void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound(edict_t *entity, int channel, char *sample, int volume,
				   float attenuation);

void SV_DropClient(bool crash);

void SV_SendClientMessages();
void SV_ClearDatagram();

int SV_ModelIndex(char *name);

void SV_SetIdealPitch();

void SV_AddUpdates();

void SV_ClientThink();
void SV_AddClientToServer(struct qsocket_s *ret);

void SV_ClientPrintf(char *fmt, ...);
void SV_BroadcastPrintf(char *fmt, ...);

void SV_Physics();

bool SV_CheckBottom(edict_t *ent);
bool SV_movestep(edict_t *ent, vec3_t move, bool relink);

void SV_WriteClientdataToMessage(edict_t *ent, sizebuf_t *msg);

void SV_MoveToGoal();

void SV_CheckForNewClients();
void SV_RunClients();
void SV_SaveSpawnparms();
void SV_SpawnServer(char *server, char *startspot);

#endif /* !_SERVER_H */
