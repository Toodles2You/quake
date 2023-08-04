/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include <stdio.h>

#include "quakedef.h"

#include "stb_vorbis.h"

typedef enum
{
    CD_ENABLED = 1,
    CD_PLAYING = 2,
    CD_LOOPING = 4,
    CD_PAUSED = 8,
    CD_FADING_IN = 16,
    CD_FADING_OUT = 32,
    CD_FADING_MASK = (CD_FADING_IN | CD_FADING_OUT),
    CD_PLAYING_MASK = (CD_PLAYING | CD_PAUSED | CD_LOOPING | CD_FADING_MASK),
} cdstate_t;

static cvar_t bgmfade = {"bgmfade", "0.5", false};

static cdstate_t cd_state;

static stb_vorbis *vorbis;
static short* bgm_buffer;

static char trackdir[MAX_QPATH] = "music/track%02i.ogg";
static char trackname[MAX_OSPATH];
static byte tracknum;
static FILE *trackfile;
static int trackrate;
static int trackchannels;

static double fadetime;
static byte queuenum;
static qboolean queuelooping;

static byte remap[100];

static void CDAudio_ForceStop()
{
    if (!(cd_state & CD_ENABLED))
        return;
    
    SNDDMA_BeginPainting();

    if (vorbis)
    {
        stb_vorbis_close(vorbis);
        fclose(trackfile);
        vorbis = NULL;
    }

    cd_state &= ~CD_PLAYING_MASK;

    SNDDMA_Submit();
}

static void CDAudio_FadeIn(float time)
{
    fadetime = realtime + time;
    cd_state &= ~CD_FADING_OUT;
    cd_state |= CD_FADING_IN;
}

static void CDAudio_FadeOut(float time)
{
    fadetime = realtime + time;
    cd_state &= ~CD_FADING_IN;
    cd_state |= CD_FADING_OUT;
}

void CDAudio_Play(byte track, qboolean looping)
{
    if (!(cd_state & CD_ENABLED))
        return;

    track = remap[track];

    if (track < 1)
    {
        Con_DPrintf("CDAudio: Bad track number %u\n", track);
        return;
    }

    if (cd_state & CD_PLAYING)
    {
        if (tracknum == track)
            return;
        
        if (bgmfade.value > 0.0)
        {
            if (queuenum == track)
                return;
            
            queuenum = track;
            CDAudio_FadeOut(bgmfade.value);
            return;
        }
    }

    CDAudio_ForceStop();

    sprintf(trackname, trackdir, track);

    int filelen = COM_FOpenFile(trackname, &trackfile);

    if (filelen <= 0)
    {
        Con_Printf("CDAudio: %s not found\n", trackname);
        return;
    }

    SNDDMA_BeginPainting();

    int error;
    vorbis = stb_vorbis_open_file(trackfile, false, &error, NULL);

    if (vorbis == NULL || stb_vorbis_seek_start(vorbis) < 0)
    {
        Con_Printf("CDAudio: %s is not a valid audio file (%i)\n", trackname, error);
        SNDDMA_Submit();
        return;
    }

    stb_vorbis_info info = stb_vorbis_get_info(vorbis);

    trackrate = (info.sample_rate / shm->speed);

    if (trackrate <= 0)
    {
        SNDDMA_Submit();
        return;
    }

    if (shm->channels == 1)
        trackrate *= 2;
    
    trackchannels = info.channels;

    Con_DPrintf(
        "CDAudio: %s %s\nCDAudio: %i channels, %i hz\n",
        looping ? "Looping" : "Playing",
        trackname,
        info.channels,
        info.sample_rate
    );

    if (looping)
        cd_state |= CD_LOOPING;
    else
        cd_state &= ~CD_LOOPING;
    
    cd_state &= ~CD_FADING_MASK;

    tracknum = track;
    cd_state |= CD_PLAYING;
    fadetime = realtime + 1;

    SNDDMA_Submit();
}

void CDAudio_Stop()
{
    if (bgmfade.value > 0.0 && !(cd_state & CD_FADING_OUT))
    {
        CDAudio_FadeOut(bgmfade.value);
        return;
    }

    CDAudio_ForceStop();
}

void CDAudio_Pause()
{
    if (!(cd_state & CD_ENABLED) || !(cd_state & CD_PLAYING))
        return;

    cd_state |= CD_PAUSED;
}

void CDAudio_Resume()
{
    if (!(cd_state & CD_ENABLED) || !(cd_state & CD_PAUSED))
        return;

    cd_state &= ~CD_PAUSED;
}

static void CD_f()
{
    if (Cmd_Argc() < 2)
        return;

    char *command = Cmd_Argv(1);
    int ret;
    int n;

    if (Q_strcasecmp(command, "on") == 0)
    {
        cd_state |= CD_ENABLED;
    }
    else if (Q_strcasecmp(command, "off") == 0)
    {
        CDAudio_ForceStop();
        cd_state &= ~CD_ENABLED;
    }
    else if (Q_strcasecmp(command, "reset") == 0)
    {
        cd_state |= CD_ENABLED;

        CDAudio_ForceStop();
        
        for (n = 1; n < 100; n++)
            remap[n] = n;
    }
    else if (Q_strcasecmp(command, "remap") == 0)
    {
        ret = Cmd_Argc() - 2;

        if (ret <= 0)
        {
            Con_Printf("CDAudio:\n");

            for (n = 1; n < 100; n++)
            {
                if (remap[n] != n)
                    Con_Printf("  %u -> %u\n", n, remap[n]);
            }
            return;
        }
        
        for (n = 1; n <= ret; n++)
            remap[n] = Q_atoi(Cmd_Argv(n + 1));
    }
    else if (Q_strcasecmp(command, "play") == 0)
    {
        CDAudio_Play((byte)Q_atoi(Cmd_Argv(2)), false);
    }
    else if (Q_strcasecmp(command, "loop") == 0)
    {
        CDAudio_Play((byte)Q_atoi(Cmd_Argv(2)), true);
    }
    else if (Q_strcasecmp(command, "stop") == 0)
    {
        CDAudio_Stop();
    }
    else if (Q_strcasecmp(command, "pause") == 0)
    {
        CDAudio_Pause();
    }
    else if (Q_strcasecmp(command, "resume") == 0)
    {
        CDAudio_Resume();
    }
    else if (Q_strcasecmp(command, "eject") == 0)
    {
        CDAudio_ForceStop();
    }
    else if (Q_strcasecmp(command, "info") == 0)
    {
        Con_Printf("CDAudio:\n99 tracks\n");

        if (cd_state & CD_PLAYING)
            Con_Printf("Currently %s %s\n", (cd_state & CD_LOOPING) ? "looping" : "playing", trackname);
        else if (cd_state & CD_PAUSED)
            Con_Printf("Paused %s %s\n", (cd_state & CD_LOOPING) ? "looping" : "playing", trackname);
        
        Con_Printf("Volume is %f\n", bgmvolume.value);
    }
}

void CDAudio_Update()
{
    if (!(cd_state & CD_ENABLED))
        return;

    if (!(cd_state & CD_PLAYING))
    {
        CDAudio_ForceStop();
        
        if (queuenum > 0)
        {
            CDAudio_Play(queuenum, queuelooping);
            queuenum = 0;
            queuelooping = false;
        }
    }
}

int CDAudio_Init()
{
    int n;

    cd_state = 0;
    vorbis = NULL;

    if (cls.state == ca_dedicated)
        return -1;

    if (COM_CheckParm("-nocdaudio"))
        return -1;

    if ((n = COM_CheckParm("-cddev")) != 0 && n < com_argc - 1)
    {
        strncpy(trackdir, com_argv[n + 1], sizeof(trackdir));
        trackdir[sizeof(trackdir) - 1] = 0;
    }

    bgm_buffer = calloc((size_t)bgmbuffer.value, sizeof(*bgm_buffer));

    if (!bgm_buffer)
    {
        Con_Printf("CDAudio: Failed to allocate %i bytes\n", (int)bgmbuffer.value * sizeof(*bgm_buffer));
        return -1;
    }

    for (n = 1; n < 100; n++)
        remap[n] = n;

    tracknum = 0;
    queuenum = 0;
    cd_state |= CD_ENABLED;

    Cmd_AddCommand("cd", CD_f);
    Cvar_RegisterVariable(&bgmfade);

    Con_Printf("CD Audio Initialized\n");

    return 0;
}

void CDAudio_Shutdown()
{
    SNDDMA_BeginPainting();
    
    if (bgm_buffer)
    {
        free(bgm_buffer);
        bgm_buffer = NULL;
    }
    
    if (!(cd_state & CD_ENABLED))
        return;

    CDAudio_ForceStop();
    cd_state = 0;

    SNDDMA_Submit();
}

void CDAudio_Paint(void* out, int idx, int count)
{
    if (!(cd_state & CD_PLAYING) || (cd_state & CD_PAUSED))
        return;

    int recv =
    stb_vorbis_get_samples_short_interleaved(
        vorbis,
        2,
        &bgm_buffer[0],
        count * trackrate
    );

    recv *= shm->channels;

    if (recv < count)
    {
        if ((cd_state & CD_LOOPING) && stb_vorbis_seek_start(vorbis) >= 0)
        {
            recv =
            stb_vorbis_get_samples_short_interleaved(
                vorbis,
                2,
                &bgm_buffer[recv],
                (count - recv) * trackrate
            );
        }
        else
        {
            count = recv;
            cd_state &= ~CD_PLAYING_MASK;
        }
    }

    if (count <= 0)
        return;

    short *inptr = (short *)bgm_buffer;
    int out_mask = shm->samples - 1;
    int step = trackrate;
    int bgm_vol = bgmvolume.value * 256;
    int val;

    if (cd_state & CD_FADING_OUT)
    {
        float scale = (fadetime - realtime) / 0.5;

        if (scale <= 0.0) {
            scale = 0.0;
            cd_state &= ~CD_PLAYING_MASK;
        }
        else if (scale >= 1.0) {
            scale = 1.0;
        }
        
        bgm_vol *= scale;
    }
    else if (cd_state & CD_FADING_IN)
    {
        float scale = 1.0 - ((fadetime - realtime) / 0.5);

        if (scale <= 0.0) {
            scale = 0.0;
        }
        else if (scale >= 1.0) {
            scale = 1.0;
            cd_state &= ~CD_FADING_IN;
        }
        
        bgm_vol *= scale;
    }

    if (shm->samplebits == 16)
    {
        short *outptr = (short *)out;

        while (count--)
        {
            val = ((*inptr * bgm_vol) >> 8);
            inptr += step;
            val = val + outptr[idx];
            if (val > 0x7fff)
                val = 0x7fff;
            else if (val < (short)0x8000)
                val = (short)0x8000;
            outptr[idx] = val;
            idx = (idx + 1) & out_mask;
        }
    }
    else if (shm->samplebits == 8)
    {
        unsigned char *outptr = (unsigned char *)out;
        
        while (count--)
        {
            val = ((*inptr * bgm_vol) >> 8);
            inptr += step;
            val = ((val >> 8) + (outptr[idx] - 128) + 128);
            if (val > 255)
                val = 255;
            else if (val < 0)
                val = 0;
            outptr[idx] = val;
            idx = (idx + 1) & out_mask;
        }
    }
}
