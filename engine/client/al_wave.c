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

extern bool snd_loadas8bit;
extern int snd_speed;

extern cvar_t loadas8bit;

typedef struct
{
	int rate;
	int width;
	int channels;
	int loopstart;
	int samples;
	int dataofs;
} wavinfo_t;

static byte *data_p;
static byte *iff_end;
static byte *last_chunk;
static byte *iff_data;
static int iff_chunk_len;

static int GetLittleShort (void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	data_p += 2;
	return val;
}

static int GetLittleLong (void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	val = val + (*(data_p + 2) << 16);
	val = val + (*(data_p + 3) << 24);
	data_p += 4;
	return val;
}

static void FindNextChunk (char *name)
{
	while (1)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{ // didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong ();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		//		if (iff_chunk_len > 1024*1024)
		//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);
		if (!strncmp (data_p, name, 4))
			return;
	}
}

static void FindChunk (char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}

static void DumpChunks (void)
{
#if 0
	char str[5];

	str[4] = 0;
	data_p = iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong ();
		Con_Printf ("0x%x : %sfx (%d)\n", (uintptr_t)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
#endif
}

static wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength)
{
	wavinfo_t info;
	int i;
	int format;
	int samples;

	memset (&info, 0, sizeof (info));

	if (!wav)
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk ("RIFF");
	if (!(data_p && !strncmp (data_p + 8, "WAVE", 4)))
	{
		Con_Printf ("Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;
	DumpChunks ();

	FindChunk ("fmt ");
	if (!data_p)
	{
		Con_Printf ("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	format = GetLittleShort ();
	if (format != 1)
	{
		Con_Printf ("Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort ();
	info.rate = GetLittleLong ();
	data_p += 4 + 2;
	info.width = GetLittleShort () / 8;

	// get cue chunk
	FindChunk ("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong ();
		//		Con_Printf("loopstart=%d\n", sfx->loopstart);

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!strncmp (data_p + 28, "mark", 4))
			{ // this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong (); // samples in loop
				info.samples = info.loopstart + i;
				//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk ("data");
	if (!data_p)
	{
		Con_Printf ("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error ("Sound %sfx has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}

static byte *ResampleSfx (sfx_t *sfx, int inrate, int inwidth, int outrate, byte *data)
{
	float stepscale = (float)inrate / outrate; // this is usually 0.5, 1, or 2

	int outcount = sfx->length / stepscale;
	sfx->length = outcount;
	if (sfx->loopstart != -1)
		sfx->loopstart = sfx->loopstart / stepscale;

	sfx->speed = outrate;
	if (loadas8bit.value || snd_loadas8bit)
		sfx->width = 1;
	else
		sfx->width = inwidth;
	sfx->stereo = false;

	// resample / decimate to the current source rate
	byte *resampled = malloc (sfx->length * sfx->width * (1 + sfx->stereo));

	if (stepscale == 1 && inwidth == 1 && sfx->width == 1)
	{
		// fast special case
		memcpy (resampled, data, outcount);
	}
	else
	{
		// general case
		int fracstep = stepscale * 256;
		int samplefrac = 0;
		for (int i = 0; i < outcount; i++)
		{
			int srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			int sample;
			if (inwidth == 2)
				sample = ((uint16_t *)data)[srcsample];
			else
				sample = (int)data[srcsample] << 8;
			if (sfx->width == 2)
				((uint16_t *)resampled)[i] = sample;
			else
				resampled[i] = sample >> 8;
		}
	}

	return resampled;
}

bool S_LoadSound (sfx_t *sfx)
{
	// load it in
	char filename[256];
	strcpy (filename, "sound/");
	strcat (filename, sfx->name);

	byte *data = COM_LoadTempFile (filename);
	if (!data)
		return false;

	wavinfo_t info = GetWavinfo (sfx->name, data, com_filesize);
	if (info.channels != 1)
		return false;

	float stepscale = (float)info.rate / snd_speed;
	int len = info.samples / stepscale;

	len = len * info.width * info.channels;

	sfx->length = info.samples;
	sfx->loopstart = info.loopstart;
	sfx->speed = info.rate;
	sfx->width = info.width;
	sfx->stereo = false;
	sfx->duration = (float)sfx->length / sfx->speed;

	byte *resampled = ResampleSfx (sfx, sfx->speed, sfx->width, snd_speed, data + info.dataofs);

	int format;
	if (sfx->width == 2)
		format = sfx->stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	else
		format = sfx->stereo ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;

	len = sfx->length * sfx->width * (1 + sfx->stereo);
	alBufferData (sfx->al_buffers[0], format, resampled, len, sfx->speed);

	if (sfx->loopstart != -1)
	{
		int len2 = (sfx->length - sfx->loopstart) * sfx->width * (1 + sfx->stereo);
		alBufferData (sfx->al_buffers[1], format, resampled + (len - len2), len2, sfx->speed);
	}

	free (resampled);

	return true;
}