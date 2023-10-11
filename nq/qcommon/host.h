
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
	int memsize;
} quakeparms_t;

extern quakeparms_t host_parms;

extern cvar_t sys_ticrate;
extern cvar_t sys_nostdout;
extern cvar_t developer;

extern bool host_initialized; // true if into command execution
extern double host_frametime;
extern cvar_t host_timescale;
extern byte *host_basepal;
extern byte *host_colormap;
extern int host_framecount; // incremented every frame, never reset
extern double realtime;		// not bounded in any way, changed at
							// start of every frame, never reset

void Host_ClearMemory();
void Host_ServerFrame();
void Host_InitCommands();
void Host_Init(quakeparms_t *parms);
void Host_Shutdown();
void Host_Error(char *error, ...);
void Host_EndGame(char *message, ...);
void Host_Frame(float time);
void Host_Quit_f();
void Host_ClientCommands(char *fmt, ...);
void Host_ShutdownServer(bool crash);
bool Host_IsLocalGame();
bool Host_IsLocalClient();

extern int current_skill;	// skill level for currently loaded level (in case
							//  the user changes the cvar while the level is
							//  running, this reflects the level actually in use)

extern int minimum_memory;

#endif /* !_HOST_H */
