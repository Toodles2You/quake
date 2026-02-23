/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023-2025 Justin Keller

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

#include "stb_vorbis.h"

extern ALCdevice *snd_device;
extern ALCcontext *snd_context;

cvar_t bgmvolume = {"bgmvolume", "1", CVAR_ARCHIVE};

#define BGM_BUFFERS 4
#define BGM_SAMPLES 2048

static short *data;
static int buffer_size;
static char filename[MAX_OSPATH];
static FILE *stream;
static stb_vorbis *vorbis;
static bool loop;
static int channels;
static int speed;
static ALuint al_buffers[BGM_BUFFERS];
static ALuint al_source;

void Music_Stop (void)
{
	if (!snd_context)
		return;

	alSourceStop (al_source);

	if (vorbis)
	{
		stb_vorbis_close (vorbis);
		vorbis = NULL;
	}

	if (stream)
	{
		fclose (stream);
		stream = NULL;
	}
}

static void Music_PaintBuffer (ALuint buffer)
{
	int recv = stb_vorbis_get_samples_short_interleaved (vorbis, channels, data, BGM_SAMPLES) * channels;
	if (recv < BGM_SAMPLES)
	{
		if (loop && stb_vorbis_seek_start (vorbis) >= 0)
			recv += stb_vorbis_get_samples_short_interleaved (vorbis, channels, data + recv, BGM_SAMPLES - recv);
		else
			Music_Stop ();
	}
	if (recv <= 0)
		return;
	alBufferData (buffer, (channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, recv * 2, speed);
	alSourceQueueBuffers (al_source, 1, &buffer);
}

void Music_Play (int track, bool looping)
{
	if (!snd_context)
		return;

	if (track < 0)
	{
		Music_Stop ();
		return;
	}

	Music_Stop ();

	sprintf (filename, "music/track%02i.ogg", track);

	int filelen = COM_FOpenFile (filename, &stream);
	if (filelen <= 0)
	{
		Con_Printf ("%s not found\n", filename);
		return;
	}

	int error;
	vorbis = stb_vorbis_open_file (stream, false, &error, NULL);
	if (!vorbis || stb_vorbis_seek_start (vorbis) < 0)
	{
		fclose (stream);
		stream = NULL;
		Con_Printf ("%s is not a valid track\n", filename);
		return;
	}

	stb_vorbis_info info = stb_vorbis_get_info (vorbis);

	loop = looping;
	channels = info.channels;
	speed = info.sample_rate;

	alSourceRewind (al_source);
    alSourcei (al_source, AL_BUFFER, 0);

	for (int i = 0; i < BGM_BUFFERS; i++)
		Music_PaintBuffer (al_buffers[i]);

	alSourcePlay (al_source);

	Con_Printf ("%s %s\n", loop ? "Looping" : "Playing", filename);
	Con_DPrintf ("%i channels, %i hz\n", info.channels, info.sample_rate);
}

void Music_Pause (void)
{
	if (!snd_context)
		return;

	alSourcePause (al_source);
}

void Music_Resume (void)
{
	if (!snd_context)
		return;

	ALint state;
	alGetSourcei (al_source, AL_SOURCE_STATE, &state);
	if (state != AL_PAUSED)
		return;

	alSourcePlay (al_source);
}

void Music_Update (void)
{
	if (!snd_context)
		return;

	alSourcef (al_source, AL_GAIN, bgmvolume.value);

	ALint state;
	alGetSourcei (al_source, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING)
		return;

	ALint processed;
	alGetSourcei (al_source, AL_BUFFERS_PROCESSED, &processed);

	if (!processed)
		return;

	while (processed)
	{
		ALuint buffer;
		alSourceUnqueueBuffers (al_source, 1, &buffer);
		Music_PaintBuffer (buffer);
		processed--;
	}
}

static void CD_f (void)
{
	if (Cmd_Argc () >= 2)
	{
		char *command = Cmd_Argv (1);

		if (strcasecmp (command, "off") == 0)
			Music_Stop ();
		else if (strcasecmp (command, "reset") == 0)
			Music_Stop ();
		else if (strcasecmp (command, "play") == 0)
			Music_Play (atoi (Cmd_Argv (2)), false);
		else if (strcasecmp (command, "loop") == 0)
			Music_Play (atoi (Cmd_Argv (2)), true);
		else if (strcasecmp (command, "stop") == 0)
			Music_Stop ();
		else if (strcasecmp (command, "pause") == 0)
			Music_Pause ();
		else if (strcasecmp (command, "resume") == 0)
			Music_Resume ();
		else if (strcasecmp (command, "eject") == 0)
			Music_Stop ();

		if (strcasecmp (command, "info") != 0)
			return;
	}

	ALint state;
	alGetSourcei (al_source, AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING)
		Con_Printf ("Currently %s %s\n", loop ? "looping" : "playing", filename);

	Con_Printf ("Volume is %f\n", bgmvolume.value);
}

void Music_Init (void)
{
	if (!snd_context)
		return;

	Cmd_AddCommand (src_client, "cd", CD_f);
	Cvar_RegisterVariable (src_client, &bgmvolume);

	data = Hunk_AllocName (BGM_SAMPLES * 2, "bgm");

	alGenBuffers (BGM_BUFFERS, al_buffers);
	alGenSources (1, &al_source);
}

void Music_Shutdown (void)
{
	Music_Stop ();

	alDeleteSources (1, &al_source);
	alDeleteBuffers (BGM_BUFFERS, al_buffers);
}
