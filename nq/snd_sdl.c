/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2005-2012 O.Sezer <sezero@users.sourceforge.net>
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

#include <SDL2/SDL.h>

static qboolean snd_inited = false;

static int buffersize;
static int samplesize;

static SDL_AudioDeviceID device;

static void SNDDMA_DataCallback(void* user, byte* stream, int len)
{
    if (!snd_inited)
    {
        memset(stream, 0, len);
        return;
    }

    int pos = shm->samplepos * samplesize;

    if (pos >= buffersize)
        shm->samplepos = pos = 0;

    int buffer_end = buffersize - pos;
    int len1 = len;
    int len2 = 0;

    if (len1 > buffer_end)
    {
        len1 = buffer_end;
        len2 = len - len1;
    }

    memcpy(stream, shm->buffer + pos, len1);
    CDAudio_Paint(stream, 0, len1);

    if (len2 <= 0)
    {
        shm->samplepos += len1 / samplesize;
    }
    else
    {
        memcpy(stream + len1, shm->buffer, len2);
        CDAudio_Paint(stream, len1, len2);
        shm->samplepos = len2 / samplesize;
    }

    if (shm->samplepos >= buffersize)
        shm->samplepos = 0;
}

void SNDDMA_Shutdown()
{
    snd_inited = false;

    if (device != 0)
    {
        SDL_PauseAudioDevice(device, SDL_TRUE);
        SDL_CloseAudioDevice(device);
    }
    
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    
    if (shm)
    {
        if (shm->buffer)
        {
            free(shm->buffer);
            shm->buffer = NULL;
        }
        
        shm = NULL;
    }
}

static void SNDDMA_CheckParms(SDL_AudioSpec *desired)
{
    int i;
    int bits;

    if ((i = COM_CheckParm("-sndbits")) != 0)
    {
        bits = atoi(com_argv[i + 1]);
    }

    if (bits == 8)
    {
        desired->format = AUDIO_U8;
    }
    else if (bits == 16)
    {
        desired->format = AUDIO_S16LSB;
    }
    else
    {
        desired->format = AUDIO_U8;
    }

    if ((i = COM_CheckParm("-sndspeed")) != 0)
    {
        desired->freq = atoi(com_argv[i + 1]);
    }
    else
    {
        desired->freq = 11025;
    }

    if ((i = COM_CheckParm("-sndmono")) != 0)
    {
		desired->channels = 1;
    }
	else if ((i = COM_CheckParm("-sndstereo")) != 0)
    {
		desired->channels = 2;
    }
    else
    {
        desired->channels = 2;
    }

    if (desired->freq <= 11025)
    {
        desired->samples = 256;
    }
    else if (desired->freq <= 22050)
    {
        desired->samples = 512;
    }
    else if (desired->freq <= 44100)
    {
        desired->samples = 1024;
    }
    else
    {
        desired->samples = 2048;
    }
}

qboolean SNDDMA_Init()
{
    if (snd_inited)
        return true;

    snd_inited = false;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        Con_Printf("Error couldn't init SDL audio: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_AudioSpec desired, spec;
    memset(&desired, 0, sizeof(desired));

    shm = &sn;
    shm->splitbuffer = false;

    SNDDMA_CheckParms(&desired);

    desired.callback = SNDDMA_DataCallback;
    desired.userdata = NULL;

    device = SDL_OpenAudioDevice(
        NULL,
        SDL_FALSE,
        &desired,
        &spec,
        SDL_AUDIO_ALLOW_ANY_CHANGE
    );

    if (device == 0)
    {
        Con_Printf("Error couldn't open audio device: %s\n", SDL_GetError());
        SNDDMA_Shutdown();
        return false;
    }

    int samples = (spec.samples * spec.channels) * 10;
    int val;
    /* Make it a power of two. */
    if (samples & (samples - 1))
    {
        val = 1;
        while (val < samples)
            val <<= 1;
        samples = val;
    }

	shm->soundalive = true;
    shm->speed = spec.freq;
    shm->samplebits = spec.format & 255;
    shm->channels = SDL_clamp(spec.channels, 1, 2);
    shm->samples = samples;
    shm->samplepos = 0;
    shm->submission_chunk = 512;

    samplesize = shm->samplebits / 8;
    buffersize = shm->samples * samplesize;
    shm->buffer = calloc(buffersize, 1);

    if (!shm->buffer)
    {
        Con_Printf("Error couldn't allocate sound buffer\n");
        SNDDMA_Shutdown();
        return false;
    }

    SDL_PauseAudioDevice(device, SDL_FALSE);

    snd_inited = true;
    
    return true;
}

qboolean SNDDMA_BeginPainting()
{
    if (!snd_inited)
        return false;
    
    SDL_LockAudioDevice(device);

    return true;
}

int SNDDMA_GetDMAPos()
{
    if (!SNDDMA_BeginPainting())
        return 0;
    
    return shm->samplepos;
}

void SNDDMA_Submit()
{
    if (!snd_inited)
        return;
    
    SDL_UnlockAudioDevice(device);
}
