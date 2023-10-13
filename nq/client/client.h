
#ifndef _CLIENT_H
#define _CLIENT_H

typedef struct
{
	int length;
	char map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct
{
	char name[MAX_SCOREBOARDNAME];
	float entertime;
	int frags;
	int colors; // two 4 bit fields
	byte translations[VID_GRADES * 256];
} scoreboard_t;

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

#define NAME_LENGTH 64

//
// client_state_t should hold all pieces of the client state
//

#define SIGNONS 4 // signon messages to receive before connected

#define MAX_DLIGHTS 32

typedef struct
{
	vec3_t origin;
	float radius;
	float die;		// stop lighting after this time
	float decay;	// drop this each second
	float minlight; // don't add when contributing less
	int key;
} dlight_t;

#define MAX_BEAMS 24

typedef struct
{
	int entity;
	struct model_s *model;
	float endtime;
	vec3_t start, end;
} beam_t;

#define MAX_EFRAGS 640

#define MAX_MAPSTRING 2048
#define MAX_DEMOS 8
#define MAX_DEMONAME 16

typedef enum
{
	ca_dedicated,	 // a dedicated server with no ability to start a client
	ca_disconnected, // full screen console with no connection
	ca_connected	 // valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
	cactive_t state;

	// personalization data sent to server
	char mapstring[MAX_QPATH];
	char spawnparms[MAX_MAPSTRING]; // to restart a level

	// demo loop control
	int demonum;						 // -1 = don't play demos
	char demos[MAX_DEMOS][MAX_DEMONAME]; // when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	bool demorecording;
	bool demoplayback;
	bool timedemo;
	int forcetrack; // -1 = use normal cd track
	FILE *demofile;
	int td_lastframe;	// to meter out one message a frame
	int td_startframe;	// host_framecount at start
	float td_starttime; // realtime at second frame of timedemo

	// connection information
	int signon; // 0 to SIGNONS
	struct qsocket_s *netcon;
	sizebuf_t message; // writing buffer to send to server

} client_static_t;

extern client_static_t cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int movemessages; // since connecting to this server
					  // throw out the first couple, so the player
					  // doesn't accidentally do something the
					  // first frame
	usercmd_t cmd;	  // last command sent to the server

	// information for local display
	int stats[MAX_CL_STATS]; // health, etc
	int items;				 // inventory bit flags
	float item_gettime[32];	 // cl.time of aquiring item, for blinking
	float faceanimtime;		 // use anim frame if cl.time < this

	cshift_t cshifts[NUM_CSHIFTS];		// color shifts for damage, powerups
	cshift_t prev_cshifts[NUM_CSHIFTS]; // and content types

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  The server sets punchangle when
	// the view is temporarliy offset, and an angle reset commands at the start
	// of each level and after teleporting.
	vec3_t mviewangles[2]; // during demo playback viewangles is lerped
						   // between these
	vec3_t viewangles;

	vec3_t mvelocity[2]; // update by server, used for lean+bob
						 // (0 is newest)
	vec3_t velocity;	 // lerped between mvelocity[0] and [1]

	vec3_t punchangle; // temporary offset

	// pitch drifting vars
	float idealpitch;
	float pitchvel;
	bool nodrift;
	float driftmove;
	double laststop;

	float viewheight;
	float crouch; // local amount for smoothing stepups

	bool paused; // send over by server
	bool onground;
	bool inwater;

	int intermission;	// don't change view angle, full screen, etc
	int completed_time; // latched at intermission start

	double mtime[2]; // the timestamp of last two messages
	double time;	 // clients view of time, should be between
					 // servertime and oldservertime to generate
					 // a lerp point for other data
	double oldtime;	 // previous cl.time, time-oldtime is used
					 // to decay light values and smooth step ups

	float last_received_message; // (realtime) for net trouble icon

	//
	// information that is static for the entire time connected to a server
	//
	struct model_s *model_precache[MAX_MODELS];
	struct cmodel_s *cmodel_precache[MAX_MODELS];
	struct sfx_s *sound_precache[MAX_SOUNDS];

	char levelname[40]; // for display on solo scoreboard
	int viewentity;		// cl_entitites[cl.viewentity] = player
	int maxclients;
	int gametype;

	// refresh related state
	struct model_s *worldmodel; // cl_entitites[0].model
	struct efrag_s *free_efrags;
	int num_entities; // held in cl_entities array
	int num_statics;  // held in cl_staticentities array
	entity_t viewent; // the gun model

	int cdtrack, looptrack; // cd audio

	// frag scoreboard
	scoreboard_t *scores; // [cl.maxclients]
} client_state_t;

//
// cvars
//
extern cvar_t cl_name;
extern cvar_t cl_color;

extern cvar_t cl_upspeed;
extern cvar_t cl_forwardspeed;
extern cvar_t cl_backspeed;
extern cvar_t cl_sidespeed;

extern cvar_t cl_movespeedkey;

extern cvar_t cl_yawspeed;
extern cvar_t cl_pitchspeed;

extern cvar_t cl_anglespeedkey;

extern cvar_t cl_autofire;

extern cvar_t cl_shownet;
extern cvar_t cl_nolerp;
extern cvar_t cl_sbar;
extern cvar_t cl_maxfps;

extern cvar_t cl_pitchdriftspeed;
extern cvar_t lookspring;
extern cvar_t lookstrafe;
extern cvar_t sensitivity;

extern cvar_t m_pitch;
extern cvar_t m_yaw;
extern cvar_t m_forward;
extern cvar_t m_side;

extern cvar_t in_mouse;

#define MAX_TEMP_ENTITIES 64	// lightning bolts, etc
#define MAX_STATIC_ENTITIES 128 // torches, etc

extern client_state_t cl;

// FIXME, allocate dynamically
extern efrag_t cl_efrags[MAX_EFRAGS];
extern entity_t cl_entities[MAX_EDICTS];
extern entity_t cl_static_entities[MAX_STATIC_ENTITIES];
extern lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
extern dlight_t cl_dlights[MAX_DLIGHTS];
extern entity_t cl_temp_entities[MAX_TEMP_ENTITIES];
extern beam_t cl_beams[MAX_BEAMS];

//=============================================================================

//
// cl_main
//
dlight_t *CL_AllocDlight(int key);
void CL_DecayLights();

void CL_Init();

void CL_EstablishConnection(char *host);

void CL_Disconnect();
void CL_Disconnect_f();
void CL_NextDemo();

#define MAX_VISEDICTS 256
extern int cl_numvisedicts;
extern entity_t *cl_visedicts[MAX_VISEDICTS];

//
// cl_input
//
typedef struct
{
	int down[2]; // key nums holding it down
	int state;	 // low bit is down state
} kbutton_t;

extern kbutton_t in_mlook, in_klook;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;

void CL_InitInput();
void CL_SendCmd();
void CL_SendMove(usercmd_t *cmd);

void CL_ParseTEnt();
void CL_UpdateTEnts();

void CL_ClearState();

int CL_ReadFromServer();
void CL_BaseMove(usercmd_t *cmd);

float CL_KeyState(kbutton_t *key);
char *Key_KeynumToString(int keynum);

//
// cl_demo.c
//
void CL_StopPlayback();
int CL_GetMessage();

void CL_Stop_f();
void CL_Record_f();
void CL_PlayDemo_f();
void CL_TimeDemo_f();

//
// cl_parse.c
//
void CL_ParseServerMessage();
void CL_NewTranslation(int slot);

//
// view
//
void V_StartPitchDrift();
void V_StopPitchDrift();

void V_RenderView();
void V_UpdatePalette();
void V_ParseDamage();
void V_SetContentsColor(int contents);

//
// cl_tent
//
void CL_InitTEnts();
void CL_SignonReply();

#endif /* !_CLIENT_H */