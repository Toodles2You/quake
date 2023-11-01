
#ifndef _BOTHDEF_H
#define _BOTHDEF_H

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MINIMUM_MEMORY 0x550000
#define MINIMUM_MEMORY_LEVELPAK (MINIMUM_MEMORY + 0x100000)

enum
{
    PITCH,
    YAW,
    ROLL,
};

#define MAX_QPATH 64   // max length of a quake game pathname
#define MAX_OSPATH 128 // max length of a filesystem pathname

#define ON_EPSILON 0.1 // point on plane side epsilon

#define MAX_MSGLEN 8000   // max length of a reliable message
#define MAX_DATAGRAM 1024 // max length of unreliable message

#define	MAXPRINTMSG	4096

//===========================================
// per-level limits

#define MIN_EDICTS 600
#define MAX_LIGHTSTYLES 64
#define MAX_MODELS 256 // these are sent over the net as bytes
#define MAX_SOUNDS 256 // so they cannot be blindly increased

#define SAVEGAME_COMMENT_LENGTH 39

#define MAX_STYLESTRING 64

//===========================================
// stats are integers communicated to the client by the server

enum
{
    STAT_HEALTH = 0,
    STAT_FRAGS,
    STAT_WEAPON,
    STAT_AMMO,
    STAT_ARMOR,
    STAT_WEAPONFRAME,
    STAT_SHELLS,
    STAT_NAILS,
    STAT_ROCKETS,
    STAT_CELLS,
    STAT_ACTIVEWEAPON,
    STAT_TOTALSECRETS,
    STAT_TOTALMONSTERS,
    STAT_SECRETS, // bumped on client side by svc_foundsecret
    STAT_MONSTERS, // bumped by svc_killedmonster

    MAX_CL_STATS = 32,
};

//===========================================
// stock defines

enum
{
    IT_SHOTGUN          = 1 << 0,
    IT_SUPER_SHOTGUN    = 1 << 1,
    IT_NAILGUN          = 1 << 2,
    IT_SUPER_NAILGUN    = 1 << 3,
    IT_GRENADE_LAUNCHER = 1 << 4,
    IT_ROCKET_LAUNCHER  = 1 << 5,
    IT_LIGHTNING        = 1 << 6,
    IT_SUPER_LIGHTNING  = 1 << 7,
    IT_SHELLS           = 1 << 8,
    IT_NAILS            = 1 << 9,
    IT_ROCKETS          = 1 << 10,
    IT_CELLS            = 1 << 11,
    IT_AXE              = 1 << 12,
    IT_ARMOR1           = 1 << 13,
    IT_ARMOR2           = 1 << 14,
    IT_ARMOR3           = 1 << 15,
    IT_SUPERHEALTH      = 1 << 16,
    IT_KEY1             = 1 << 17,
    IT_KEY2             = 1 << 18,
    IT_INVISIBILITY     = 1 << 19,
    IT_INVULNERABILITY  = 1 << 20,
    IT_SUIT             = 1 << 21,
    IT_QUAD             = 1 << 22,

    IT_SIGIL1           = 1 << 28,
    IT_SIGIL2           = 1 << 29,
    IT_SIGIL3           = 1 << 30,
    IT_SIGIL4           = 1 << 31,
};

//===========================================
// rogue changed and added defines

enum
{
    RIT_SHELLS              = 1 << 7,
    RIT_NAILS               = 1 << 8,
    RIT_ROCKETS             = 1 << 9,
    RIT_CELLS               = 1 << 10,
    RIT_AXE                 = 1 << 11,
    RIT_LAVA_NAILGUN        = 1 << 12,
    RIT_LAVA_SUPER_NAILGUN  = 1 << 13,
    RIT_MULTI_GRENADE       = 1 << 14,
    RIT_MULTI_ROCKET        = 1 << 15,
    RIT_PLASMA_GUN          = 1 << 16,
    RIT_ARMOR1              = 8388608,
    RIT_ARMOR2              = 16777216,
    RIT_ARMOR3              = 33554432,
    RIT_LAVA_NAILS          = 67108864,
    RIT_PLASMA_AMMO         = 134217728,
    RIT_MULTI_ROCKETS       = 268435456,
    RIT_SHIELD              = 536870912,
    RIT_ANTIGRAV            = 1073741824,
    RIT_SUPERHEALTH         = 2147483648,
};

//===========================================
// hipnotic added defines

enum
{
    HIT_PROXIMITY_GUN_BIT   = 16,
    HIT_MJOLNIR_BIT         = 7,
    HIT_LASER_CANNON_BIT    = 23,
    HIT_PROXIMITY_GUN       = 1 << HIT_PROXIMITY_GUN_BIT,
    HIT_MJOLNIR             = 1 << HIT_MJOLNIR_BIT,
    HIT_LASER_CANNON        = 1 << HIT_LASER_CANNON_BIT,
    HIT_WETSUIT             = 1 << (HIT_LASER_CANNON_BIT + 2),
    HIT_EMPATHY_SHIELDS     = 1 << (HIT_LASER_CANNON_BIT + 3),
};

//===========================================

enum
{
	MOVETYPE_NONE = 0, // never moves
	MOVETYPE_ANGLENOCLIP,
	MOVETYPE_ANGLECLIP,
	MOVETYPE_WALK, // gravity
	MOVETYPE_STEP, // gravity, special edge handling
	MOVETYPE_FLY,
	MOVETYPE_TOSS, // gravity
	MOVETYPE_PUSH, // no clip to world, push and crush
	MOVETYPE_NOCLIP,
	MOVETYPE_FLYMISSILE, // extra size to monsters
	MOVETYPE_BOUNCE,
	MOVETYPE_BOUNCEMISSILE, // bounce w/o gravity
	MOVETYPE_FOLLOW, // track movement of aiment
};

enum
{
	SOLID_NOT = 0, // no interaction with other objects
	SOLID_TRIGGER, // touch on edge, but not blocking
	SOLID_BBOX, // touch on edge, block
	SOLID_SLIDEBOX, // touch on edge, but not an onground
	SOLID_BSP, // bsp clip, touch on edge, block
};

enum
{
	EF_BRIGHTFIELD	= 1 << 0,
	EF_MUZZLEFLASH	= 1 << 1,
	EF_BRIGHTLIGHT	= 1 << 2,
	EF_DIMLIGHT		= 1 << 3,
	EF_NODRAW		= 1 << 7,
};

//===========================================

#define MAX_SCOREBOARD 16
#define MAX_SCOREBOARDNAME 32

//===========================================

#include "../qcommon/common.h"
#include "../qcommon/sys.h"
#include "../qcommon/zone.h"
#include "mathlib.h"
#include "../qcommon/cvar.h"
#include "../qcommon/net.h"
#include "../qcommon/protocol.h"
#include "../qcommon/cmd.h"
#include "../qcommon/crc.h"
#include "../qcommon/cmodel.h"
#include "../qcommon/host.h"

#endif /* !_BOTHDEF_H */
