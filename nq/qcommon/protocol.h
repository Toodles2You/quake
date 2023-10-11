
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#define PROTOCOL_VERSION 15

enum
{
// if the high bit of the servercmd is set, the low bits are fast update flags:
    U_MOREBITS      = 1 << 0,
    U_ORIGIN1       = 1 << 1,
    U_ORIGIN2       = 1 << 2,
    U_ORIGIN3       = 1 << 3,
    U_ANGLE2        = 1 << 4,
    U_NOLERP        = 1 << 5, // don't interpolate movement
    U_FRAME         = 1 << 6,
    U_SIGNAL        = 1 << 7, // just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
    U_ANGLE1        = 1 << 8,
    U_ANGLE3        = 1 << 9,
    U_MODEL         = 1 << 10,
    U_COLORMAP      = 1 << 11,
    U_SKIN          = 1 << 12,
    U_EFFECTS       = 1 << 13,
    U_LONGENTITY    = 1 << 14,

    SU_VIEWHEIGHT   = 1 << 0,
    SU_IDEALPITCH   = 1 << 1,
    SU_PUNCH1       = 1 << 2,
    SU_PUNCH2       = 1 << 3,
    SU_PUNCH3       = 1 << 4,
    SU_VELOCITY1    = 1 << 5,
    SU_VELOCITY2    = 1 << 6,
    SU_VELOCITY3    = 1 << 7,
    SU_AIMENT       = 1 << 8,
    SU_ITEMS        = 1 << 9,
    SU_ONGROUND     = 1 << 10, // no data follows, the bit is it
    SU_INWATER      = 1 << 11, // no data follows, the bit is it
    SU_WEAPONFRAME  = 1 << 12,
    SU_ARMOR        = 1 << 13,
    SU_WEAPON       = 1 << 14,
};

// a sound with no channel is a local only sound
enum
{
    SND_VOLUME      = 1 << 0, // a byte
    SND_ATTENUATION = 1 << 1, // a byte
    SND_LOOPING     = 1 << 2, // a long
};

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT 22

// game types sent by serverinfo
// these determine which intermission screen plays
enum
{
    GAME_COOP = 0,
    GAME_DEATHMATCH,
};

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
enum
{
    svc_bad = 0,
    svc_nop,
    svc_disconnect,
    svc_updatestat, // [byte] [long]
    svc_version, // [long] server version
    svc_setview, // [short] entity number
    svc_sound, // <see code>
    svc_time, // [float] server time
    svc_print, // [string] null terminated string
    svc_stufftext, // [string] stuffed into client's console buffer
                     // the string should be \n terminated
    svc_setangle, // [angle3] set the view angle to this absolute value

    svc_serverinfo,// [long] version
                            // [string] signon string
                            // [string]..[0]model cache
                            // [string]...[0]sounds cache
    svc_lightstyle, // [byte] [string]
    svc_updatename, // [byte] [string]
    svc_updatefrags, // [byte] [short]
    svc_clientdata, // <shortbits + data>
    svc_stopsound, // <see code>
    svc_updatecolors, // [byte] [byte]
    svc_particle, // [vec3] <variable>
    svc_damage,

    svc_spawnstatic,
    svc_spawnbinary,
    svc_spawnbaseline,

    svc_temp_entity,

    svc_setpause, // [byte] on / off
    svc_signonnum, // [byte]  used for the signon sequence

    svc_centerprint, // [string] to put in center of the screen

    svc_killedmonster,
    svc_foundsecret,

    svc_spawnstaticsound, // [coord3] [byte] samp [byte] vol [byte] aten

    svc_intermission, // [string] music
    svc_finale, // [string] music [string] text

    svc_cdtrack, // [byte] track [byte] looptrack
    svc_sellscreen,

    svc_cutscene,
};

//
// client to server
//
enum
{
    clc_bad = 0,
    clc_nop,
    clc_disconnect,
    clc_move, // [usercmd_t]
    clc_stringcmd, // [string] message
};

//
// temp entity events
//
enum
{
    TE_SPIKE = 0,
    TE_SUPERSPIKE,
    TE_GUNSHOT,
    TE_EXPLOSION,
    TE_TAREXPLOSION,
    TE_LIGHTNING1,
    TE_LIGHTNING2,
    TE_WIZSPIKE,
    TE_KNIGHTSPIKE,
    TE_LIGHTNING3,
    TE_LAVASPLASH,
    TE_TELEPORT,
    TE_EXPLOSION2,
    TE_BEAM,
};

typedef struct
{
    vec3_t viewangles;

    // intended velocities
    float forwardmove;
    float sidemove;
    float upmove;
} usercmd_t;

typedef struct
{
    vec3_t origin;
    vec3_t angles;
    int modelindex;
    int frame;
    int colormap;
    int skin;
    int effects;
} entity_state_t;

#endif /* !_PROTOCOL_H */
