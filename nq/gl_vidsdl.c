/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023 Justin Keller

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

#include "quakedef.h"

#include <GL/glx.h>
#include <SDL2/SDL.h>

enum {
    WARP_WIDTH = 320,
    WARP_HEIGHT = 200,
};

int texture_extension_number = 1;
float gldepthmin, gldepthmax;

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

SDL_Window *window = NULL;

qboolean isPermedia = false;
qboolean gl_mtexable = false;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];
unsigned char d_15to8table[65536];

cvar_t gl_ztrick = {"gl_ztrick", "1"};

static void (*qglColorTableEXT)(int, int, int, int, int, const void *);
static void (*qgl3DfxSetPaletteEXT)(GLuint *);

static SDL_GLContext context = NULL;

static qboolean vidmode_active = false;
static qboolean is8bit = false;
static int scr_width, scr_height;
static float vid_gamma = 1.0;

static cvar_t vid_mode = {"vid_mode", "0", false};

void VID_Shutdown()
{
    /* IN_DeactivateGrabs(); */

    if (context)
        SDL_GL_DeleteContext(context);

    if (window)
        SDL_DestroyWindow(window);

    vidmode_active = false;
    context = NULL;
    window = NULL;

    SDL_Quit();
}

static void signal_handler(int sig)
{
    printf("Received signal %d, exiting...\n", sig);
    Sys_Quit();
    exit(0);
}

static void VID_InitSignals()
{
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGTRAP, signal_handler);
    signal(SIGIOT, signal_handler);
    signal(SIGBUS, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(unsigned char *palette)
{
    /* VID_SetPalette(palette); */
}

void VID_SetPalette(unsigned char *palette)
{
    byte *pal;
    unsigned r, g, b;
    unsigned v;
    int r1, g1, b1;
    int j, k, l, m;
    unsigned short i;
    unsigned *table;
    FILE *f;
    char s[255];
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

    for (i = 0; i < (1 << 15); i++)
    {
        /* Maps
        000000000000000
        000000000011111 = Red  = 0x1F
        000001111100000 = Blue = 0x03E0
        111110000000000 = Grn  = 0x7C00
        */
        r = ((i & 0x1F) << 3) + 4;
        g = ((i & 0x03E0) >> 2) + 4;
        b = ((i & 0x7C00) >> 7) + 4;
        pal = (unsigned char *)d_8to24table;
        for (v = 0, k = 0, bestdist = 10000 * 10000; v < 256; v++, pal += 4)
        {
            r1 = (int)r - (int)pal[0];
            g1 = (int)g - (int)pal[1];
            b1 = (int)b - (int)pal[2];
            dist = (r1 * r1) + (g1 * g1) + (b1 * b1);
            if (dist < bestdist)
            {
                k = v;
                bestdist = dist;
            }
        }
        d_15to8table[i] = k;
    }
}

static void GL_CheckMultiTextureExtensions()
{
    void *prjobj;

    if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex"))
    {
        Con_Printf("Found GL_SGIS_multitexture...\n");

        if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL)
        {
            Con_Printf("Unable to open symbol list for main program.\n");
            return;
        }

        qglMTexCoord2fSGIS = (void *)dlsym(prjobj, "glMTexCoord2fSGIS");
        qglSelectTextureSGIS = (void *)dlsym(prjobj, "glSelectTextureSGIS");

        if (qglMTexCoord2fSGIS && qglSelectTextureSGIS)
        {
            Con_Printf("Multitexture extensions found.\n");
            gl_mtexable = true;
        }
        else
            Con_Printf("Symbol not found, disabled.\n");

        dlclose(prjobj);
    }
}

/*
===============
GL_Init
===============
*/
void GL_Init()
{
    gl_vendor = glGetString(GL_VENDOR);
    Con_Printf("GL_VENDOR: %s\n", gl_vendor);
    gl_renderer = glGetString(GL_RENDERER);
    Con_Printf("GL_RENDERER: %s\n", gl_renderer);

    gl_version = glGetString(GL_VERSION);
    Con_Printf("GL_VERSION: %s\n", gl_version);
    gl_extensions = glGetString(GL_EXTENSIONS);
    // Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

    //	Con_Printf ("%s %s\n", gl_renderer, gl_version);

    GL_CheckMultiTextureExtensions();

    glClearColor(1, 0, 0, 0);
    glCullFace(GL_FRONT);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.666);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel(GL_FLAT);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    glEnable (GL_STENCIL_TEST);
    glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc (GL_ALWAYS, 0, 1);
	glStencilMask (1);
    glClearStencil (0);
    glStencilMask (0);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering(int *x, int *y, int *width, int *height)
{
    extern cvar_t gl_clear;

    *x = *y = 0;
    *width = scr_width;
    *height = scr_height;

    //    if (!wglMakeCurrent( maindc, baseRC ))
    //		Sys_Error ("wglMakeCurrent failed");

    //	glViewport (*x, *y, *width, *height);
}

void GL_EndRendering()
{
    glFlush();
    SDL_GL_SwapWindow(window);
}

static void VID_CheckGamma(unsigned char *pal)
{
    float f, inf;
    unsigned char palette[768];
    int i;

    if ((i = COM_CheckParm("-gamma")) == 0)
    {
        if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
            (gl_vendor && strstr(gl_vendor, "3Dfx")))
            vid_gamma = 1;
        else
            vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
    }
    else
        vid_gamma = Q_atof(com_argv[i + 1]);

    for (i = 0; i < 768; i++)
    {
        f = pow((pal[i] + 1) / 256.0, vid_gamma);
        inf = f * 255 + 0.5;
        if (inf < 0)
            inf = 0;
        if (inf > 255)
            inf = 255;
        palette[i] = inf;
    }

    memcpy(pal, palette, sizeof(palette));
}

static void VID_SetMode(int width, int height, qboolean fullscreen)
{
    vidmode_active = false;
    int display = SDL_GetWindowDisplayIndex(window);

    if (display < 0)
        return;

    int num_vidmodes = SDL_GetNumDisplayModes(display);
    SDL_DisplayMode vidmode;

    if (num_vidmodes <= 0 || SDL_GetDesktopDisplayMode(display, &vidmode) < 0)
        return;

    SDL_SetWindowFullscreen(window, 0);

    /* Are we going fullscreen? If so, let's change video mode. */
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
            if (SDL_GetDisplayMode(display, i, &current) < 0)
                continue;

            if (width > current.w ||
                height > current.h)
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

        /* change to the mode */
        if (best_fit != -1)
        {
            vidmode_active = true;
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }
    }

    SDL_SetWindowDisplayMode(window, &vidmode);
}

static void VID_CheckParms(int *width, int *height, qboolean *fullscreen)
{
    int i;
    
    if ((i = COM_CheckParm("-window")) != 0)
    {
        *fullscreen = false;
    }
    else
    {
        *fullscreen = true;
    }

    if ((i = COM_CheckParm("-width")) != 0)
    {
        *width = atoi(com_argv[i + 1]);
    }
    else
    {
        *width = 640;
    }

    if ((i = COM_CheckParm("-height")) != 0)
    {
        *height = atoi(com_argv[i + 1]);
    }
    else
    {
        *height = 480;
    }

    if ((i = COM_CheckParm("-conwidth")) != 0)
    {
        /* Make it a multiple of eight. */
        vid.conwidth = Q_atoi(com_argv[i + 1]) & 0xFFF8;
        vid.conwidth = SDL_max(vid.conwidth, 320);
    }
    else
    {
        vid.conwidth = 640;
    }

    if ((i = COM_CheckParm("-conheight")) != 0)
    {
        vid.conheight = SDL_max(Q_atoi(com_argv[i + 1]), 200);
    }
    else
    {
        vid.conheight = vid.conwidth * (*height) / (*width);
    }
}

void VID_Init(unsigned char *palette)
{
    int i;
    int width, height;
    qboolean fullscreen;

    Cvar_RegisterVariable(&vid_mode);
    Cvar_RegisterVariable(&gl_ztrick);

    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));

    VID_CheckParms(&width, &height, &fullscreen);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Error couldn't open SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_version version;
    SDL_GetVersion(&version);
    Con_Printf("Using SDL Version %d.%d.%d\n", version.major, version.minor, version.patch);

    /* Set the window attributes. */
    SDL_GL_ResetAttributes();

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) < 0
        || SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE) < 0
        || SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 1) < 0
        || SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1) < 0)
    {
        fprintf(stderr, "Error couldn't set window attributes: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    window = SDL_CreateWindow(
        "Quake",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

    if (!window)
    {
        fprintf(stderr, "Error couldn't create window: %s\n", SDL_GetError());
        exit(1);
    }

    VID_SetMode(width, height, fullscreen);

    context = SDL_GL_CreateContext(window);

    if (!context)
    {
        fprintf(stderr, "Error couldn't create OpenGL context: %s\n", SDL_GetError());
        exit(1);
    }

    scr_width = width;
    scr_height = height;

    vid.height = vid.conheight = SDL_min(vid.conheight, height);
    vid.width = vid.conwidth = SDL_min(vid.conwidth, width);
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 2;

    VID_InitSignals();

    GL_Init();

    char gldir[MAX_OSPATH];
    sprintf(gldir, "%s/glquake", com_gamedir);
    Sys_mkdir(gldir);

    VID_CheckGamma(palette);
    VID_SetPalette(palette);

    Con_SafePrintf("Video mode %dx%d initialized.\n", width, height);

    /* Force a surface cache flush. */
    vid.recalc_refdef = 1;

    SDL_ShowWindow(window);
}
