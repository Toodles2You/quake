
#ifndef _CLIENT_H
#define _CLIENT_H

// player_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
typedef struct
{
	int messagenum; // all player's won't be updated each frame

	double state_time; // not the same as the packet time,
					   // because player commands come asyncronously
	usercmd_t command; // last command for prediction

	vec3_t origin;
	vec3_t viewangles; // only for demos, not from server
	vec3_t velocity;
	int weaponframe;

	int modelindex;
	int frame;
	int skinnum;
	int effects;

	int flags; // dead, gib, etc

	float waterjumptime;
	int onground; // -1 = in air, else pmove entity number
	int oldbuttons;
} player_state_t;

#define MAX_SCOREBOARDNAME 16

typedef struct player_info_s
{
	int userid;
	char userinfo[MAX_INFO_STRING];

	// scoreboard information
	char name[MAX_SCOREBOARDNAME];
	float entertime;
	int frags;
	int ping;
	byte pl;
} player_info_t;

typedef struct
{
	// generated on client side
	usercmd_t cmd;		// cmd that generated the frame
	double senttime;	// time cmd was sent off
	int delta_sequence; // sequence number to delta from, -1 = full update

	// received from server
	double receivedtime;					 // time message was received, or -1
	player_state_t playerstate[MAX_CLIENTS]; // message received that reflects performing
											 // the usercmd
	packet_entities_t packet_entities;
	bool invalid; // true if the packet_entities delta was invalid
} frame_t;

typedef struct
{
	int length;
	char map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct
{
	int destcolor[3];
	int percent; // 0-256
} cshift_t;

enum
{
	CSHIFT_CONTENTS = 0,
	CSHIFT_DAMAGE,
	CSHIFT_BONUS,
	CSHIFT_POWERUP,
	NUM_CSHIFTS,
};

#define MAX_DLIGHTS 32

typedef struct
{
	int key; // so entities can reuse same entry
	vec3_t origin;
	float radius;
	float die;		// stop lighting after this time
	float decay;	// drop this each second
	float minlight; // don't add when contributing less
	float color[4];
} dlight_t;

#define MAX_EFRAGS 640

#define MAX_DEMOS 8
#define MAX_DEMONAME 16

typedef enum
{
	ca_dedicated,	 // a dedicated server with no ability to start a client
	ca_demostart,	 // starting up a demo
	ca_disconnected, // full screen console with no connection
	ca_connected,	 // valid netcon, talking to a server
	ca_onserver,	 // processing data lists, donwloading, etc
	ca_active,		 // everything is in, so frames can be rendered
} cactive_t;

// download type
typedef enum
{
	dl_none,
	dl_model,
	dl_sound,
	dl_single,
} dltype_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
	// connection information
	cactive_t state;

	// network stuff
	netchan_t netchan;

	// private userinfo for sending to masterless servers
	char userinfo[MAX_INFO_STRING];

	char servername[MAX_OSPATH]; // name of server from original connect
	int protocol;

	int qport;

	FILE *download; // file transfer from server
	char downloadtempname[MAX_OSPATH];
	char downloadname[MAX_OSPATH];
	int downloadnumber;
	dltype_t downloadtype;
	int downloadpercent;

	// demo loop control
	int demonum;						 // -1 = don't play demos
	char demos[MAX_DEMOS][MAX_DEMONAME]; // when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	bool demorecording;
	bool demoplayback;
	bool timedemo;
	FILE *demofile;
	float td_lastframe; // to meter out one message a frame
	int td_startframe;	// host_framecount at start
	float td_starttime; // host_time at second frame of timedemo

	int challenge;

	float latency; // rolling average
} client_static_t;

extern client_static_t cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int servercount; // server identification for prespawns

	char serverinfo[MAX_SERVERINFO_STRING];

	int parsecount;	   // server message counter
	int validsequence; // this is the sequence number of the last good
					   // packetentity_t we got.  If this is 0, we can't
					   // render a frame yet
	int movemessages;  // since connecting to this server
					   // throw out the first couple, so the player
					   // doesn't accidentally do something the
					   // first frame

	double last_ping_request; // while showing scoreboard
	double last_servermessage;

	// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK] = cmd
	frame_t frames[UPDATE_BACKUP];

	// information for local display
	int stats[MAX_CL_STATS]; // health, etc
	float item_gettime[32];	 // cl.time of aquiring item, for blinking
	float faceanimtime;		 // use anim frame if cl.time < this

	cshift_t cshifts[NUM_CSHIFTS];		// color shifts for damage, powerups
	cshift_t prev_cshifts[NUM_CSHIFTS]; // and content types

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  And only reset at level change
	// and teleport times
	vec3_t viewangles;

	// the client simulates or interpolates movement to get these values
	double time; // this is the time value that the client is rendering at.  allways <= host_time
	double frametime;
	vec3_t simorg;
	vec3_t simvel;
	vec3_t simangles;

	// pitch drifting vars
	float pitchvel;
	bool nodrift;
	float driftmove;
	double laststop;

	float crouch; // local amount for smoothing stepups

	bool paused; // send over by server

	float punchangle; // temporar yview kick from weapon firing

	int intermission;	// don't change view angle, full screen, etc
	int completed_time; // latched ffrom time at intermission start

	//
	// information that is static for the entire time connected to a server
	//
	char model_name[MAX_MODELS][MAX_QPATH];
	char sound_name[MAX_SOUNDS][MAX_QPATH];

	struct model_s *model_precache[MAX_MODELS];
	struct cmodel_s *cmodel_precache[MAX_MODELS];
	struct sfx_s *sound_precache[MAX_SOUNDS];

	char levelname[40]; // for display on solo scoreboard
	int playernum;
	int userid;

	// refresh related state
	struct model_s *worldmodel; // cl_entitites[0].model
	struct efrag_s *free_efrags;
	int num_entities; // stored bottom up in cl_entities array
	int num_statics;  // stored top down in cl_entitiers

	int cdtrack; // cd audio

	entity_t viewent; // weapon model

	// all player information
	player_info_t players[MAX_CLIENTS];
} client_state_t;

//
// cvars
//

extern cvar_t cl_upspeed;
extern cvar_t cl_forwardspeed;
extern cvar_t cl_backspeed;
extern cvar_t cl_sidespeed;

extern cvar_t cl_movespeedkey;

extern cvar_t cl_yawspeed;
extern cvar_t cl_pitchspeed;

extern cvar_t cl_anglespeedkey;

extern cvar_t cl_shownet;
extern cvar_t cl_sbar;
extern cvar_t cl_maxfps;
extern cvar_t cl_showfps;

extern cvar_t cl_pitchdriftspeed;
extern cvar_t lookspring;
extern cvar_t lookstrafe;
extern cvar_t sensitivity;

extern cvar_t m_pitch;
extern cvar_t m_yaw;
extern cvar_t m_forward;
extern cvar_t m_side;

extern cvar_t in_mouse;

#define MAX_STATIC_ENTITIES 128 // torches, etc

extern client_state_t cl;

// FIXME, allocate dynamically
extern entity_state_t cl_baselines[MIN_EDICTS];
extern efrag_t cl_efrags[MAX_EFRAGS];
extern entity_t cl_static_entities[MAX_STATIC_ENTITIES];
extern lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
extern dlight_t cl_dlights[MAX_DLIGHTS];

//
// cl_main.c
//
dlight_t *CL_AllocDlight (int key);
void CL_DecayLights (void);

void CL_Init (void);
void Host_WriteConfiguration (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

void CL_CheckForResend (void);
void CL_BeginServerConnect (void);

#define MAX_VISEDICTS 256
extern int cl_numvisedicts, cl_oldnumvisedicts;
extern entity_t *cl_visedicts, *cl_oldvisedicts;
extern entity_t cl_visedicts_list[2][MAX_VISEDICTS];

//
// cl_input.c
//
typedef struct
{
	int down[2]; // key nums holding it down
	int state;	 // low bit is down state
} kbutton_t;

extern kbutton_t in_mlook;
extern kbutton_t in_strafe;

void CL_InitInput (void);
void CL_SendCmd (void);

void CL_ParseTEnt (bool quakeworld);
void CL_UpdateTEnts (void);

void CL_ClearState (void);

void CL_ReadPackets (void);

char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_StopPlayback (void);
bool CL_GetMessage (void);
void CL_WriteDemoCmd (usercmd_t *pcmd);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_ReRecord_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

//
// cl_parse.c
//
#define NET_TIMINGS 256
#define NET_TIMINGSMASK 255
extern int packet_latency[NET_TIMINGS];
int CL_CalcNet (void);
void CL_ParseServerMessage (void);
bool CL_CheckOrDownloadFile (char *filename);
void CL_NextUpload (void);
void CL_StartUpload (byte *data, int size);
void CL_StopUpload (void);

//
// view.c
//
void V_StartPitchDrift (void);
void V_StopPitchDrift (void);

void V_RenderView (void);
void V_UpdatePalette (void);
void V_ParseDamage (void);
void V_SetContentsColor (int contents);
void V_CalcBlend (void);

//
// cl_tent.c
//
void CL_InitTEnts (void);
void CL_ClearTEnts (void);

//
// cl_ents.c
//
void CL_SetSolidPlayers (int playernum);
void CL_SetUpPlayerPrediction (bool dopred);
void CL_EmitEntities (void);
void CL_ClearProjectiles (void);
void CL_ParseProjectiles (void);
void CL_ParsePacketEntities (bool delta);
void CL_SetSolidEntities (void);
void CL_ParsePlayerinfo (void);

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictPlayers (void);
void CL_PredictUsercmd (player_state_t *from, player_state_t *to, usercmd_t *u);

#endif /* !_CLIENT_H */
