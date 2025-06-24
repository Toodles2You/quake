/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2025 Justin Keller

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
===========================================================================
*/

#include "clientdef.h"

sfx_t *cl_sfx_menu1;
sfx_t *cl_sfx_menu2;
sfx_t *cl_sfx_menu3;
sfx_t *cl_sfx_talk;

cvar_t volume = {"volume", "0.7", CVAR_ARCHIVE};
cvar_t loadas8bit = {"loadas8bit", "0"};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};

bool snd_loadas8bit;
int snd_speed;
ALCdevice *snd_device;
ALCcontext *snd_context;

#define MAX_CHANNELS 128
#define MAX_DYNAMIC_CHANNELS 8

static channel_t *channels;
static int num_channels;
static ALuint *al_sources;

#define MAX_SFX 512
#define MAX_BUFFERS (MAX_SFX * 2)

static sfx_t *known_sfx;
static int num_sfx;
static int low_sfx;
static ALuint *al_buffers;

#define SOUND_NOMINAL_CLIP_DIST 1000.0f

static vec3_t listener_origin;

static bool snd_ambient = true;
static sfx_t *ambient_sfx[NUM_AMBIENTS];

void S_SetAmbientActive (bool active)
{
	snd_ambient = active;
}

static void S_StopAllSounds_f (void)
{
	S_StopAllSounds ();
}

static void S_Play_f (void)
{
	static int hash = 345;
	int i;
	char name[256];
	sfx_t *sfx;

	i = 1;
	while (i < Cmd_Argc ())
	{
		if (!strrchr (Cmd_Argv (i), '.'))
		{
			strcpy (name, Cmd_Argv (i));
			strcat (name, ".wav");
		}
		else
			strcpy (name, Cmd_Argv (i));
		sfx = S_PrecacheSound (name);
		S_StartSound (hash++, 0, sfx, listener_origin, 1.0, 1.0);
		i++;
	}
}

static void S_PlayVol_f (void)
{
	static int hash = 543;
	int i;
	float vol;
	char name[256];
	sfx_t *sfx;

	i = 1;
	while (i < Cmd_Argc ())
	{
		if (!strrchr (Cmd_Argv (i), '.'))
		{
			strcpy (name, Cmd_Argv (i));
			strcat (name, ".wav");
		}
		else
			strcpy (name, Cmd_Argv (i));
		sfx = S_PrecacheSound (name);
		vol = atof (Cmd_Argv (i + 1));
		S_StartSound (hash++, 0, sfx, listener_origin, vol, 1.0);
		i += 2;
	}
}

static void S_CheckParms (void)
{
	int i;

	if ((i = COM_CheckParm ("-sndbits")) != 0)
		snd_loadas8bit = atoi (com_argv[i + 1]) <= 8;
	else
		snd_loadas8bit = false;

	if ((i = COM_CheckParm ("-sndspeed")) != 0)
		snd_speed = atoi (com_argv[i + 1]);
	else
		snd_speed = 11025;
}

static void S_CreateContext (void)
{
	if (snd_context)
		return;

	S_CheckParms ();

	const ALCchar *devicename = alcGetString (NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	snd_device = alcOpenDevice (devicename);
	if (!snd_device)
	{
		Con_Printf ("Couldn't open sound device\n");
		S_Shutdown ();
		return;
	}

	snd_context = alcCreateContext (snd_device, NULL);
	if (!snd_context || !alcMakeContextCurrent (snd_context))
	{
		Con_Printf ("Couldn't create sound context\n");
		S_Shutdown ();
		return;
	}

	alDistanceModel (AL_LINEAR_DISTANCE);
}

void S_Init (void)
{
	if (COM_CheckParm ("-nosound"))
		return;

	Con_Printf ("\nSound Initialization\n");

	Cmd_AddCommand (src_client, "play", S_Play_f);
	Cmd_AddCommand (src_client, "playvol", S_PlayVol_f);
	Cmd_AddCommand (src_client, "stopsound", S_StopAllSounds_f);

	Cvar_RegisterVariable (src_client, &volume);
	Cvar_RegisterVariable (src_client, &loadas8bit);
	Cvar_RegisterVariable (src_client, &ambient_level);
	Cvar_RegisterVariable (src_client, &ambient_fade);

	if (host_parms.memsize < 0x800000)
	{
		Cvar_Set (src_client, "loadas8bit", "1");
		Con_Printf ("loading all sounds as 8bit\n");
	}

	S_CreateContext ();

	known_sfx = Hunk_AllocName (MAX_SFX * sizeof (sfx_t) + MAX_BUFFERS * sizeof (ALuint), "sfx_t");
	al_buffers = (ALuint *)(known_sfx + MAX_SFX);
	num_sfx = 0;
	alGenBuffers (MAX_BUFFERS, al_buffers);

	channels = Hunk_AllocName (MAX_CHANNELS * (sizeof (channel_t) + sizeof (ALuint)), "channel_t");
	al_sources = (ALuint *)(channels + MAX_CHANNELS);
	num_channels = 0;
	alGenSources (MAX_CHANNELS, al_sources);

	for (int i = 0; i < MAX_CHANNELS; i++)
		channels[i].al_source = al_sources[i];

	cl_sfx_menu1 = S_PrecacheSound ("misc/menu1.wav");
	cl_sfx_menu2 = S_PrecacheSound ("misc/menu2.wav");
	cl_sfx_menu3 = S_PrecacheSound ("misc/menu3.wav");
	cl_sfx_talk = S_PrecacheSound ("misc/talk.wav");

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav");

	S_StopAllSounds ();
}

void S_Shutdown (void)
{
	S_StopAllSounds ();

	alDeleteBuffers (MAX_BUFFERS, al_buffers);
	alDeleteSources (MAX_CHANNELS, al_sources);

	if (snd_context)
	{
		alcMakeContextCurrent (NULL);
		alcDestroyContext (snd_context);
		snd_context = NULL;
	}

	if (snd_device)
	{
		alcCloseDevice (snd_device);
		snd_device = NULL;
	}
}

static sfx_t *S_FindName (char *name)
{
	int i;
	sfx_t *sfx;

	if (!name)
		Sys_Error ("S_FindName: NULL\n");

	if (strlen (name) >= MAX_QPATH)
		Sys_Error ("Sound name too long: %s", name);

	// see if already loaded
	for (i = 0; i < num_sfx; i++)
		if (!strcmp (known_sfx[i].name, name))
			return &known_sfx[i];

	if (num_sfx == MAX_SFX)
		Sys_Error ("S_FindName: out of sfx_t");

	sfx = &known_sfx[i];
	strcpy (sfx->name, name);
	sfx->al_buffers[0] = al_buffers[i * 2];
	sfx->al_buffers[1] = al_buffers[i * 2 + 1];

	if (!S_LoadSound (sfx))
		return NULL;

	num_sfx++;

	return sfx;
}

sfx_t *S_PrecacheSound (char *name)
{
	if (!snd_context)
		return NULL;

	return S_FindName (name);
}

// FIXME:
void S_SetSoundPaused (bool paused) {}

static channel_t *SND_PickChannel (int entnum, int entchannel)
{
	// Check for replacement sound, or find the best one to replace
	int best = -1;
	double bestremaining = 3600.0;
	for (int i = NUM_AMBIENTS; i < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; i++)
	{
		channel_t *chan = channels + i;

		// channel 0 never overrides
		if (entchannel != 0 && chan->entnum == entnum && (entchannel == -1 || chan->entchannel == entchannel))
		{ // allways override sound from same entity
			best = i;
			break;
		}

		// don't let monster sounds override player sounds
		if (chan->entnum == cl.playernum + 1 && entnum != cl.playernum + 1 && chan->sfx)
			continue;

		double remaining = chan->finished - realtime;
		if (remaining >= bestremaining)
			continue;

		best = i;
		bestremaining = remaining;
	}

	if (best == -1)
		return NULL;

	return channels + best;
}

static bool SND_Audible (int entnum, vec3_t origin, float falloff)
{
	// anything coming from the view entity will allways be full volume
	if (entnum == cl.playernum + 1)
		return true;

	// calculate stereo seperation and distance attenuation
	vec3_t diff;
	VectorSubtract (origin, listener_origin, diff);

	float dist = VectorNormalize (diff) * falloff;

	// add in distance effect
	float scale = (1.0f - dist);

	return scale > 0.0f;
}

static void SND_Spatialize (channel_t *chan)
{
	// anything coming from the view entity will allways be full volume
	if (chan->entnum == cl.playernum + 1)
	{
		alSourcef (chan->al_source, AL_ROLLOFF_FACTOR, 0);
		alSourcef (chan->al_source, AL_REFERENCE_DISTANCE, SOUND_NOMINAL_CLIP_DIST);
		alSourcefv (chan->al_source, AL_POSITION, listener_origin);
	}
	else
	{
		alSourcef (chan->al_source, AL_ROLLOFF_FACTOR, 1);
		alSourcef (chan->al_source, AL_REFERENCE_DISTANCE, 0);
		alSourcefv (chan->al_source, AL_POSITION, chan->origin);
	}
}

void SND_Stop (channel_t *chan)
{
	if (!chan->sfx)
		return;

	alSourceStop (chan->al_source);
	alSourcei (chan->al_source, AL_LOOPING, AL_FALSE);

	ALuint buffers[2];
	if (chan->looping == LOOP_LOOP)
		alSourceUnqueueBuffers (chan->al_source, 1, buffers);
	else if (chan->looping == LOOP_INTRO)
		alSourceUnqueueBuffers (chan->al_source, 2, buffers);
	else
		alSourceUnqueueBuffers (chan->al_source, 1, buffers);

	chan->looping = LOOP_NO;
	chan->sfx = NULL;
}

void SND_Play (channel_t *chan)
{
	chan->looping = (chan->sfx->loopstart != -1) ? LOOP_INTRO : LOOP_NO;
	chan->finished = realtime + chan->sfx->duration;

	alSourceRewind (chan->al_source);
	alSourcei (chan->al_source, AL_BUFFER, 0);

	if (chan->looping)
		alSourceQueueBuffers (chan->al_source, 2, chan->sfx->al_buffers);
	else
		alSourceQueueBuffers (chan->al_source, 1, chan->sfx->al_buffers);

	alSourcei (chan->al_source, AL_LOOPING, AL_FALSE);
	alSourcef (chan->al_source, AL_PITCH, 1);
	alSourcef (chan->al_source, AL_GAIN, chan->volume);
	alSourcef (chan->al_source, AL_MAX_DISTANCE, 1.0f / chan->falloff);

	alSourcePlay (chan->al_source);
}

void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	if (!snd_context)
		return;

	if (!sfx)
		return;

	if (!SND_Audible (entnum, origin, attenuation / SOUND_NOMINAL_CLIP_DIST))
		return;

	// pick a channel to play on
	channel_t *chan = SND_PickChannel (entnum, entchannel);
	if (!chan)
		return;

	SND_Stop (chan);

	VectorCopy (origin, chan->origin);
	chan->volume = fvol;
	chan->falloff = attenuation / SOUND_NOMINAL_CLIP_DIST;
	chan->entnum = entnum;
	chan->entchannel = entchannel;

	// spatialize
	SND_Spatialize (chan);

	chan->sfx = sfx;

	// if an identical sound has also been started this frame, offset the pos
	// a bit to keep it from just making the first one louder
	for (int i = NUM_AMBIENTS; i < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; i++)
	{
		channel_t *check = channels + i;

		if (check == chan)
			continue;

		if (check->sfx != chan->sfx || check->finished != chan->finished)
			continue;

		float skip = 0.1f * ((float)rand () / RAND_MAX);
		alSourcef (chan->al_source, AL_SEC_OFFSET, skip);
		break;
	}

	SND_Play (chan);
}

void S_StopSound (int entnum, int entchannel)
{
	for (int i = 0; i < MAX_DYNAMIC_CHANNELS; i++)
		if (channels[i].entnum == entnum && channels[i].entchannel == entchannel)
		{
			SND_Stop (channels + i);
			return;
		}
}

void S_StopAllSounds ()
{
	if (!snd_context)
		return;

	for (int i = 0; i < MAX_CHANNELS; i++)
		SND_Stop (channels + i);

	num_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS; // no statics
}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	if (!sfx)
		return;

	if (num_channels == MAX_CHANNELS)
		return;

#if 0 // doesn't really matter
	if (cache->loopstart == -1)
	{
		Con_Printf ("Sound %s not looped\n", sfx->name);
		return;
	}
#endif

	channel_t *chan = channels + num_channels;
	num_channels++;

	SND_Stop (chan);

	chan->sfx = sfx;
	VectorCopy (origin, chan->origin);
	chan->volume = fvol;
	chan->falloff = (attenuation / 64.0f) / SOUND_NOMINAL_CLIP_DIST;

	SND_Spatialize (chan);
	SND_Play (chan);
}

static void S_UpdateAmbientSounds (void)
{
	// calc ambient sound levels
	mleaf_t *leaf = (snd_ambient && cl.cmodel_precache[1]) ? CMod_PointInLeaf (listener_origin, cl.cmodel_precache[1]) : NULL;

	for (int i = 0; i < NUM_AMBIENTS; i++)
	{
		channel_t *chan = channels + i;

		if (!ambient_sfx[i])
			continue;

		chan->sfx = ambient_sfx[i];
		chan->entnum = cl.playernum + 1;

		float vol = leaf ? (leaf->ambient_sound_level[i] / 255.0f) : 0;

		// don't adjust volume too fast
		if (chan->volume < vol)
		{
			chan->volume += (ambient_fade.value / 255.0f) * host_frametime;
			if (chan->volume > vol)
				chan->volume = vol;
		}
		else if (chan->volume > vol)
		{
			chan->volume -= (ambient_fade.value / 255.0f) * host_frametime;
			if (chan->volume < vol)
				chan->volume = vol;
		}

		ALint state;
		alGetSourcei (chan->al_source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING && chan->volume <= 0)
		{
			alSourcePause (chan->al_source);
			continue;
		}

		alSourcef (chan->al_source, AL_GAIN, chan->volume * ambient_level.value);
		SND_Spatialize (chan);

		if (state != AL_PLAYING)
		{
			alSourcei (chan->al_source, AL_BUFFER, chan->sfx->al_buffers[1]);
			alSourcei (chan->al_source, AL_LOOPING, AL_TRUE);
			alSourcePlay (chan->al_source);
		}
	}
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update (vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	if (!snd_context)
		return;

	alListenerf (AL_GAIN, volume.value);

	VectorCopy (origin, listener_origin);
	alListenerfv (AL_POSITION, listener_origin);

	// FIXME: orientation may be wrong due to different handedness
	// it's not noticable in it's current implementation
	ALfloat orientation[6];
	VectorCopy (forward, orientation);
	VectorCopy (up, (orientation + 3));
	alListenerfv (AL_ORIENTATION, orientation);

	// update general area ambient sound sources
	S_UpdateAmbientSounds ();

	// update spatialization for static and dynamic sounds
	for (int i = NUM_AMBIENTS; i < num_channels; i++)
	{
		channel_t *chan = channels + i;
		if (!chan->sfx)
			continue;

		if (chan->looping == LOOP_INTRO)
		{
			ALint processed;
			alGetSourcei (chan->al_source, AL_BUFFERS_PROCESSED, &processed);
			// FIXME: handle both already having been processed
			if (processed == 1)
			{
				ALuint buffer;
				alSourceUnqueueBuffers (chan->al_source, 1, &buffer);
				alSourcei (chan->al_source, AL_LOOPING, AL_TRUE);
				chan->looping = LOOP_LOOP;
			}
		}

		ALint state;
		alGetSourcei (chan->al_source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			SND_Stop (chan);
			continue;
		}

		SND_Spatialize (chan); // respatialize channel

		// TODO:
		// try to combine static sounds with a previous channel of the same
		// sound effect so we don't mix five torches every frame
	}
}

void S_LocalSound (sfx_t *sfx)
{
	if (!snd_context)
		return;
	if (!sfx)
		return;
	S_StartSound (cl.playernum + 1, -1, sfx, vec3_origin, 1, 1);
}

void S_InitBase (void)
{
	low_sfx = num_sfx;
}

void S_ClearAll (void)
{
	if (!snd_context)
		return;

	for (int i = low_sfx + 1; i < num_sfx; i++)
		memset (known_sfx + i, 0, sizeof (sfx_t));

	num_sfx = low_sfx;
}

void S_Print (void)
{
	int i;
	sfx_t *sfx;

	Con_Printf ("Cached sfx:\n");

	if (!snd_context)
		return;

	for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++)
		Con_Printf ("%u : %s\n", i, sfx->name);
}
