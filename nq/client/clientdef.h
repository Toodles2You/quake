
#ifndef _CLIENTDEF_H
#define _CLIENTDEF_H

#include "../qcommon/bothdef.h"

#include "bspfile.h"
#include "vid.h"
#include "wad.h"
#include "wadlib.h"
#include "draw.h"
#include "screen.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "client.h"
#include "input.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "cdaudio.h"
#include "glquake.h"

extern bool noclip_anglehack;

extern cvar_t chase_active;

void Chase_Init();
void Chase_Reset();
void Chase_Update();

#endif /* !_CLIENTDEF_H */
