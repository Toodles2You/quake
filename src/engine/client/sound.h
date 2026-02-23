
#ifndef _SOUND_H
#define _SOUND_H

#include <AL/al.h>
#include <AL/alc.h>

enum
{
	LOOP_NO,
	LOOP_INTRO,
	LOOP_LOOP,
};

typedef struct sfx_s
{
	char name[MAX_QPATH];
	int length;
	int loopstart;
	int speed;
	int width;
	bool stereo;
	float duration;
	ALuint al_buffers[2];
} sfx_t;

typedef struct
{
	sfx_t *sfx;
	double finished;
	int looping;
	int entnum;
	int entchannel;
	vec3_t origin;
	float falloff;
	float volume;
	bool audible;
	ALuint al_source;
} channel_t;

void S_Init (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds (void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

sfx_t *S_PrecacheSound (char *sample);

void S_SetSoundPaused (bool paused);

// ====================================================================
// User-setable variables
// ====================================================================

extern cvar_t volume;

void S_LocalSound (sfx_t *sfx);
bool S_LoadSound (sfx_t *s);
void S_SetAmbientActive (bool active);
void S_InitBase (void);
void S_ClearAll (void);
void S_Print (void);

extern sfx_t *cl_sfx_menu1;
extern sfx_t *cl_sfx_menu2;
extern sfx_t *cl_sfx_menu3;
extern sfx_t *cl_sfx_talk;

#endif /* !_SOUND_H */
