
#ifndef _HOST_H
#define _HOST_H

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char *basedir;
	char *cachedir; // for development over ISDN lines
	int argc;
	char **argv;
	void *membase;
	size_t memsize;
} quakeparms_t;

extern quakeparms_t host_parms;

extern cvar_t rcon_password;
extern cvar_t rcon_address;
extern cvar_t password;
extern cvar_t spectator_password;
extern cvar_t sys_ticrate;
extern cvar_t developer;

extern bool host_initialized; // true if into command execution
extern double host_frametime;
extern cvar_t host_timescale;
extern int host_framecount; // incremented every frame, never reset
extern double realtime;		// not bounded in any way, changed at
							// start of every frame, never reset

void Host_ClearMemory (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown (void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (double time);
void Host_Quit_f (void);
void Host_InitServer (void);
void Host_ShutdownServer (bool crash);
bool Host_IsLocalGame (void);
bool Host_IsLocalClient (int userid);
bool Host_IsPaused (void);

extern int current_skill; // skill level for currently loaded level (in case
						  //  the user changes the cvar while the level is
						  //  running, this reflects the level actually in use)

typedef enum
{
	RD_NONE,
	RD_CLIENT,
	RD_PACKET
} redirect_t;

#endif /* !_HOST_H */
