/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023-2024 Justin Keller

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

#include <sys/signal.h>
#include <dlfcn.h>

#include "clientdef.h"

#include <GL/glx.h>
#include <SDL2/SDL.h>

int texture_extension_number = 1;

SDL_Window *window = NULL;

bool gl_mtexable = false;

unsigned int d_8to24table[256];

static const char *gl_vendor;
static const char *gl_renderer;
static const char *gl_version;
static const char *gl_extensions;

static SDL_GLContext context = NULL;

static int scr_width, scr_height;

void VID_Shutdown (void)
{
	if (context)
		SDL_GL_DeleteContext (context);

	if (window)
		SDL_DestroyWindow (window);

	context = NULL;
	window = NULL;

	SDL_Quit ();
}

static void signal_handler (int sig)
{
	printf ("Received signal %d, exiting...\n", sig);
	Sys_Quit ();
	exit (0);
}

static void VID_InitSignals (void)
{
	signal (SIGHUP, signal_handler);
	signal (SIGINT, signal_handler);
	signal (SIGQUIT, signal_handler);
	signal (SIGILL, signal_handler);
	signal (SIGTRAP, signal_handler);
	signal (SIGIOT, signal_handler);
	signal (SIGBUS, signal_handler);
	signal (SIGFPE, signal_handler);
	signal (SIGSEGV, signal_handler);
	signal (SIGTERM, signal_handler);
}

static void VID_SetPalette (unsigned char *palette)
{
	byte *pal;
	unsigned int r, g, b;
	unsigned int v;
	int r1, g1, b1;
	int k;
	int i;
	unsigned int *table;
	int dist, bestdist;

	//
	// 8 8 8 encoding
	//
	pal = palette;
	table = d_8to24table;
	for (i = 0; i < 256; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;

		v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff; // 255 is transparent
}

static void GL_CheckMultiTextureExtensions (void)
{
	void *prjobj;

	if (strstr (gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm ("-nomtex"))
	{
		Con_Printf ("Found GL_SGIS_multitexture...\n");

		if ((prjobj = dlopen (NULL, RTLD_LAZY)) == NULL)
		{
			Con_Printf ("Unable to open symbol list for main program.\n");
			return;
		}

		qglMTexCoord2fSGIS = (void *)dlsym (prjobj, "glMTexCoord2fSGIS");
		qglSelectTextureSGIS = (void *)dlsym (prjobj, "glSelectTextureSGIS");

		if (qglMTexCoord2fSGIS && qglSelectTextureSGIS)
		{
			Con_Printf ("Multitexture extensions found.\n");
			gl_mtexable = true;
		}
		else
			Con_Printf ("Symbol not found, disabled.\n");

		dlclose (prjobj);
	}
}

static void GL_Init (void)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);
	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	if (strlen (gl_extensions) > 2048)
		Con_Printf ("GL_EXTENSIONS: A LOT\n");
	else
		Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

	GL_CheckMultiTextureExtensions ();

	glClearColor (0.5, 0.5, 0.5, 1);
	glCullFace (GL_FRONT);
	glEnable (GL_TEXTURE_2D);

	glEnable (GL_ALPHA_TEST);
	glAlphaFunc (GL_GREATER, 0.333);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glEnable (GL_STENCIL_TEST);
	glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc (GL_ALWAYS, 0, 1);
	glStencilMask (1);
	glClearStencil (0);
	glStencilMask (0);
}

void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;
}

void GL_EndRendering (void)
{
	glFlush ();
	SDL_GL_SwapWindow (window);
}

static void VID_CheckGamma (unsigned char *pal)
{
	float f, inf;
	unsigned char palette[768];
	int i;

	float vid_gamma;
	if ((i = COM_CheckParm ("-gamma")))
		vid_gamma = atof (com_argv[i + 1]);
	else
		vid_gamma = 1;

	for (i = 0; i < 768; i++)
	{
		f = pow ((pal[i] + 1) / 256.0, vid_gamma);
		inf = f * 255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	memcpy (pal, palette, sizeof (palette));
}

static void VID_SetMode (int width, int height, bool fullscreen)
{
	int display = SDL_GetWindowDisplayIndex (window);

	if (display < 0)
		return;

	int num_vidmodes = SDL_GetNumDisplayModes (display);
	SDL_DisplayMode vidmode;

	if (num_vidmodes <= 0 || SDL_GetDesktopDisplayMode (display, &vidmode) < 0)
		return;

	SDL_SetWindowFullscreen (window, 0);

	// change video mode
	if (fullscreen)
	{
		SDL_DisplayMode current;
		int best_dist = 9999999;
		int best_fit = -1;
		int dist;
		int x, y;
		int i;

		for (i = 0; i < num_vidmodes; i++)
		{
			if (SDL_GetDisplayMode (display, i, &current) < 0)
				continue;

			if (width > current.w || height > current.h)
				continue;

			x = width - current.w;
			y = height - current.h;
			dist = (x * x) + (y * y);

			if (dist < best_dist)
			{
				best_dist = dist;
				best_fit = i;
				vidmode = current;
			}
		}

		// change to the mode
		if (best_fit != -1)
			SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN);
	}

	SDL_SetWindowDisplayMode (window, &vidmode);
}

static void VID_CheckParms (int *width, int *height, bool *fullscreen)
{
	int i;

	if ((i = COM_CheckParm ("-window")) != 0)
		*fullscreen = false;
	else
		*fullscreen = true;

	if ((i = COM_CheckParm ("-width")) != 0)
		*width = atoi (com_argv[i + 1]);
	else
		*width = 640;

	if ((i = COM_CheckParm ("-height")) != 0)
		*height = atoi (com_argv[i + 1]);
	else
		*height = 480;

	if ((i = COM_CheckParm ("-conwidth")) != 0)
	{
		// make it a multiple of eight
		vid.conwidth = atoi (com_argv[i + 1]) & 0xFFF8;
		vid.conwidth = SDL_max (vid.conwidth, 320);
	}
	else
	{
		vid.conwidth = 640;
	}

	if ((i = COM_CheckParm ("-conheight")) != 0)
		vid.conheight = SDL_max (atoi (com_argv[i + 1]), 200);
	else
		vid.conheight = vid.conwidth * (*height) / (*width);
}

void VID_Init (char *palette)
{
	extern cvar_t gl_ztrick;

	Cvar_RegisterVariable (src_client, &gl_ztrick);

	byte *host_basepal = (byte *)COM_LoadHunkFile (palette);
	if (!host_basepal)
		Sys_Error ("Couldn't load %s", palette);

	int width, height;
	bool fullscreen;
	VID_CheckParms (&width, &height, &fullscreen);

	if (SDL_Init (SDL_INIT_VIDEO) < 0)
		Sys_Error ("Error couldn't open SDL: %s\n", SDL_GetError ());

	SDL_version version;
	SDL_GetVersion (&version);
	Con_Printf ("Using SDL Version %d.%d.%d\n", version.major, version.minor, version.patch);

	// set the window attributes
	SDL_GL_ResetAttributes ();

	if (SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0 ||
		SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0) < 0 ||
		SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) < 0)
	{
		Sys_Error ("Error couldn't set window attributes: %s\n", SDL_GetError ());
	}

	window = SDL_CreateWindow ("Quake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

	if (!window)
		Sys_Error ("Error couldn't create window: %s\n", SDL_GetError ());

	VID_SetMode (width, height, fullscreen);

	context = SDL_GL_CreateContext (window);

	if (!context)
		Sys_Error ("Error couldn't create OpenGL context: %s\n", SDL_GetError ());

	scr_width = width;
	scr_height = height;

	vid.height = vid.conheight = SDL_min (vid.conheight, height);
	vid.width = vid.conwidth = SDL_min (vid.conwidth, width);
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0f / 240.0f);

	VID_InitSignals ();

	GL_Init ();

	char gldir[MAX_OSPATH];
	sprintf (gldir, "%s/glquake", com_gamedir);
	Sys_mkdir (gldir);

	VID_CheckGamma (host_basepal);
	VID_SetPalette (host_basepal);

	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);

	// force a surface cache flush
	vid.recalc_refdef = true;

	SDL_ShowWindow (window);
}
