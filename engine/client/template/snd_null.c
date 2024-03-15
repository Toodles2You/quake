#include "clientdef.h"

cvar_t bgmvolume = {"bgmvolume", "1", CVAR_ARCHIVE};
cvar_t volume = {"volume", "0.7", CVAR_ARCHIVE};

 
void S_Init ()
{
}

void S_AmbientOff ()
{
}

void S_AmbientOn ()
{
}

void S_Shutdown ()
{
}

void S_TouchSound (char *sample)
{
}

void S_ClearBuffer ()
{
}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
}

void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol,  float attenuation)
{
}

void S_StopSound (int entnum, int entchannel)
{
}

sfx_t *S_PrecacheSound (char *sample)
{
	return NULL;
}

void S_ClearPrecache ()
{
}

void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up)
{	
}

void S_StopAllSounds (bool clear)
{
}

void S_BeginPrecaching ()
{
}

void S_EndPrecaching ()
{
}

void S_ExtraUpdate ()
{
}

void S_LocalSound (char *s)
{
}

