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
} cdstate_t;

static cdstate_t cd_state;

static stb_vorbis *vorbis;
static short* bgm_buffer;

static char trackdir[MAX_QPATH] = "music/track%02i.ogg";
static char trackname[MAX_OSPATH];
static byte tracknum;
static FILE *trackfile;
static int trackrate;
static int trackchannels;

static byte remap[100];

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
        CDAudio_Stop();
    }

    sprintf(trackname, trackdir, track);

    int filelen = COM_FOpenFile(trackname, &trackfile);

    if (filelen <= 0)
    {
        Con_Printf("CDAudio: %s not found\n", trackname);
        return;
    }

    int error;
    vorbis = stb_vorbis_open_file(trackfile, false, &error, NULL);

    if (vorbis == NULL || stb_vorbis_seek_start(vorbis) < 0)
    {
        Con_Printf("CDAudio: %s is not a valid audio file (%i)\n", trackname, error);
        return;
    }

    stb_vorbis_info info = stb_vorbis_get_info(vorbis);

    trackrate = (info.sample_rate / shm->speed);

    if (trackrate <= 0)
        return;

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

    tracknum = track;
    cd_state |= CD_PLAYING;
}

void CDAudio_Stop()
{
    if (!(cd_state & CD_ENABLED) || !(cd_state & CD_PLAYING))
        return;

    if (vorbis)
    {
        stb_vorbis_close(vorbis);
        fclose(trackfile);
        vorbis = NULL;
    }

    cd_state &= ~(CD_PLAYING | CD_PAUSED | CD_LOOPING);
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
        CDAudio_Stop();
        cd_state &= ~CD_ENABLED;
    }
    else if (Q_strcasecmp(command, "reset") == 0)
    {
        cd_state |= CD_ENABLED;

        CDAudio_Stop();
        
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
        CDAudio_Stop();
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

    cd_state |= CD_ENABLED;

    Cmd_AddCommand("cd", CD_f);

    Con_Printf("CD Audio Initialized\n");

    return 0;
}

void CDAudio_Shutdown()
{
    if (bgm_buffer)
    {
        free(bgm_buffer);
        bgm_buffer = NULL;
    }
    
    if (!(cd_state & CD_ENABLED))
        return;

    CDAudio_Stop();
    cd_state = 0;
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
            CDAudio_Stop();
        }
    }

    short *inptr = (short *)bgm_buffer;
    int out_mask = shm->samples - 1;
    int step = trackrate;
    int bgm_vol = bgmvolume.value * 256;
    int val;

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
