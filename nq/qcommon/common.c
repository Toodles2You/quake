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

#include "bothdef.h"

#define MAX_NUM_ARGVS 50
#define NUM_SAFE_ARGVS 5

static char *largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static char *argvdummy = " ";

static char *safeargvs[NUM_SAFE_ARGVS] =
{
	"-nolan",
	"-nosound",
	"-nocdaudio",
	"-nojoy",
	"-nomouse",
};

static void COM_InitFilesystem();

char com_token[1024];
int com_argc;
char **com_argv;

bool standard_quake = true;
bool rogue;
bool hipnotic;

#ifdef QUAKE_REGISTERED
// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT 339
#define PAK0_CRC 32981

// this graphic needs to be in the pak file to use registered features
static unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

bool static_registered; // only for startup check, then set
cvar_t registered = {"registered", "0"};

#else

cvar_t registered = {"registered", "1"};

#endif

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake program and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.
	
*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

#ifdef QUAKE_WINDOWS

int strncasecmp (char *s1, char *s2, int n)
{
	int             c1, c2;
	
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}
	
	return -1;
}

int strcasecmp (char *s1, char *s2)
{
	return strncasecmp (s1, s2, 99999);
}

#endif /* QUAKE_WINDOWS */

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

bool        bigendien;

int16_t (*BigShort) (int16_t l);
int16_t (*LittleShort) (int16_t l);
int32_t (*BigLong) (int32_t l);
int32_t (*LittleLong) (int32_t l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

int16_t   ShortSwap (int16_t l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

int16_t   ShortNoSwap (int16_t l)
{
	return l;
}

int32_t    LongSwap (int32_t l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int32_t)b1<<24) + ((int32_t)b2<<16) + ((int32_t)b3<<8) + b4;
}

int32_t     LongNoSwap (int32_t l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float   f;
		byte    b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte *buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte *buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte *buf = SZ_GetSpace(sb, 2);
	buf[0] = c & 0xff;
	buf[1] = c >> 8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte *buf = SZ_GetSpace(sb, 4);
	buf[0] = c & 0xff;
	buf[1] = (c >> 8) & 0xff;
	buf[2] = (c >> 16) & 0xff;
	buf[3] = c >> 24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union {
		float f;
		int32_t l;
	} dat = {f};
	MSG_WriteLong(sb, LittleLong(dat.l));
}

void MSG_WriteString (sizebuf_t *sb, char *s)
{
	if (!s)
		MSG_WriteByte(sb, '\0');
	else
		SZ_Write (sb, s, strlen(s) + 1);
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteShort(sb, (int)(f * 8));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte(sb, ((int)f * 256 / 360) & 255);
}

//
// reading functions
//
int                     msg_readcount;
bool        msg_badread;

void MSG_BeginReading ()
{
	msg_readcount = 0;
	msg_badread = false;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar ()
{
	int     c;
	
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;
	
	return c;
}

int MSG_ReadByte ()
{
	int     c;
	
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;
	
	return c;
}

int MSG_ReadShort ()
{
	int     c;
	
	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = (short)(net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8));
	
	msg_readcount += 2;
	
	return c;
}

int MSG_ReadLong ()
{
	int     c;
	
	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}
		
	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8)
	+ (net_message.data[msg_readcount+2]<<16)
	+ (net_message.data[msg_readcount+3]<<24);
	
	msg_readcount += 4;
	
	return c;
}

float MSG_ReadFloat ()
{
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	
	dat.b[0] =      net_message.data[msg_readcount];
	dat.b[1] =      net_message.data[msg_readcount+1];
	dat.b[2] =      net_message.data[msg_readcount+2];
	dat.b[3] =      net_message.data[msg_readcount+3];
	msg_readcount += 4;
	
	dat.l = LittleLong (dat.l);

	return dat.f;   
}

char *MSG_ReadString ()
{
	static char     string[2048];
	int             l,c;
	
	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

float MSG_ReadCoord ()
{
	return MSG_ReadShort() * (1.0/8);
}

float MSG_ReadAngle ()
{
	return MSG_ReadChar() * (360.0/256);
}



//===========================================================================

void SZ_Alloc (sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;
	buf->data = Hunk_AllocName (startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}


void SZ_Free (sizebuf_t *buf)
{
//      Z_Free (buf->data);
//      buf->data = NULL;
//      buf->maxsize = 0;
	buf->cursize = 0;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void    *data;
	
	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Sys_Error ("SZ_GetSpace: overflow without allowoverflow set");
		
		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);
			
		buf->overflowed = true;
		Con_Printf ("SZ_GetSpace: overflow");
		SZ_Clear (buf); 
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

void SZ_Write (sizebuf_t *buf, void *data, int length)
{
	memcpy (SZ_GetSpace(buf,length),data,length);         
}

void SZ_Print (sizebuf_t *buf, char *data)
{
	int             len;
	
	len = strlen(data)+1;

// byte * cast to keep VC++ happy
	if (buf->data[buf->cursize-1])
		memcpy ((byte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
	else
		memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,data,len); // write over trailing 0
}


//============================================================================


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char    *last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/' || *pathname=='\\')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension (char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (char *in)
{
	static char exten[8];
	int             i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i=0 ; i<7 && *in ; i++,in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (char *in, char *out)
{
	char *s, *s2;
	
	s = in + strlen(in) - 1;
	
	while (s != in && *s != '.')
		s--;
	
	for (s2 = s ; *s2 && *s2 != '/' ; s2--)
	;
	
	if (s-s2 < 2)
		strcpy (out,"?model?");
	else
	{
		s--;
		strncpy (out,s2+1, s-s2);
		out[s-s2] = 0;
	}
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
===============
COM_BeginReadPairs
===============
*/
bool COM_BeginReadPairs (byte **data)
{
	*data = COM_Parse(*data);

	if (!(*data))
		return false;
	
	if (com_token[0] != '{')
		return false;
	
	return true;
}

/*
===============
COM_ReadPair
===============
*/
bool COM_ReadPair (byte **data, char *key, char *value)
{
	*data = COM_Parse(*data);
	
	if (com_token[0] == '}')
	{
		return false;
	}

	if (!(*data))
	{
		Sys_Error("COM_ReadPair: EOF without closing brace.");
	}

	strcpy(key, com_token);

	int n = strlen(key);

	while (n && key[n - 1] == ' ')
	{
		key[n - 1] = 0;
		n--;
	}

	*data = COM_Parse(*data);
	if (!(*data))
	{
		Sys_Error("COM_ReadPair: EOF without closing brace.");
	}

	if (com_token[0] == '}')
	{
		Sys_Error("COM_ReadPair: Closing brace without data.");
	}
	
	strcpy(value, com_token);
	return true;
}

/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm (char *parm)
{
	int             i;
	
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if (!strcmp (parm,com_argv[i]))
			return i;
	}
		
	return 0;
}

#ifdef QUAKE_REGISTERED
/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
static void COM_CheckRegistered (bool modified)
{
	int             h;
	uint16_t  		check[128];
	int                     i;

	COM_OpenFile("gfx/pop.lmp", &h);
	static_registered = false;

	if (h == -1)
	{
		/* Sys_Error ("This dedicated server requires a full registered copy of Quake"); */
		Con_Printf ("Playing shareware version.\n");
		if (modified)
		{
			Sys_Error ("You must have the registered version to use modified games");
		}
		return;
	}

	Sys_FileRead (h, check, sizeof(check));
	COM_CloseFile (h);
	
	for (i=0 ; i<128 ; i++)
		if (pop[i] != (uint16_t)BigShort (check[i]))
			Sys_Error ("Corrupted data file.");

	Cvar_Set ("registered", "1");
	static_registered = true;
	Con_Printf ("Playing registered version.\n");
}
#endif


void COM_Path_f ();


/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, char **argv)
{
	bool        safe;
	int             i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	safe = false;

	for (com_argc=0 ; (com_argc<MAX_NUM_ARGVS) && (com_argc < argc) ;
		 com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!strcmp ("-safe", argv[com_argc]))
			safe = true;
	}

	if (safe)
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (i=0 ; i<NUM_SAFE_ARGVS ; i++)
		{
			largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;

	if (COM_CheckParm ("-rogue"))
	{
		rogue = true;
		standard_quake = false;
	}

	if (COM_CheckParm ("-hipnotic"))
	{
		hipnotic = true;
		standard_quake = false;
	}

	if (COM_CheckParm ("-game"))
	{
		standard_quake = false;
	}
}


/*
================
COM_Init
================
*/
void COM_Init (char *basedir)
{
	byte    swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner 
	if ( *(short *)swaptest == 1)
	{
		bigendien = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}

	Cvar_RegisterVariable (&registered);
	Cmd_AddCommand ("path", COM_Path_f);

	COM_InitFilesystem ();
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char    *va(char *format, ...)
{
	va_list         argptr;
	static char             string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	return string;  
}


/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

size_t com_filesize;


//
// in memory
//

typedef struct
{
	char name[MAX_QPATH];
	size_t filepos, filelen;
} packfile_t;

typedef struct pack_s
{
	char filename[MAX_OSPATH];
	int handle;
	size_t numfiles;
	packfile_t *files;
} pack_t;

//
// on disk
//
typedef struct
{
	char    name[56];
	int             filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int             dirofs;
	int             dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK       2048

char    com_cachedir[MAX_OSPATH];
char    com_gamedir[MAX_OSPATH];

typedef struct searchpath_s
{
	char    filename[MAX_OSPATH];
	pack_t  *pack;          // only one of filename / pack will be used
	struct searchpath_s *next;
} searchpath_t;

searchpath_t    *com_searchpaths;

/*
============
COM_Path_f

============
*/
void COM_Path_f ()
{
	searchpath_t    *s;
	
	Con_Printf ("Current search path:\n");
	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (s->pack)
		{
			Con_Printf ("%s (%lu files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_Printf ("%s\n", s->filename);
	}
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile (char *filename, void *data, size_t len)
{
	int             handle;
	char    name[MAX_OSPATH];
	
	sprintf (name, "%s/%s", com_gamedir, filename);

	handle = Sys_FileOpenWrite (name);
	if (handle == -1)
	{
		Sys_Printf ("COM_WriteFile: failed on %s\n", name);
		return;
	}
	
	Sys_Printf ("COM_WriteFile: %s\n", name);
	Sys_FileWrite (handle, data, len);
	Sys_FileClose (handle);
}


/*
============
COM_CreatePath

Only used for CopyFile
============
*/
static void    COM_CreatePath (char *path)
{
	char    *ofs;
	
	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{       // create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}


/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed.  This is for the convenience of developers using ISDN from home.
===========
*/
static void COM_CopyFile (char *netpath, char *cachepath)
{
	int in, out;
	size_t remaining, count;
	char buf[4096];
	
	remaining = Sys_FileOpenRead (netpath, &in);            
	COM_CreatePath (cachepath);     // create directories up to the cache file
	out = Sys_FileOpenWrite (cachepath);
	
	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		Sys_FileRead (in, buf, count);
		Sys_FileWrite (out, buf, count);
		remaining -= count;
	}

	Sys_FileClose (in);
	Sys_FileClose (out);    
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
static size_t COM_FindFile (char *filename, int *handle, FILE **file)
{
	searchpath_t *search;
	char netpath[MAX_OSPATH];
	char cachepath[MAX_OSPATH];
	pack_t *pak;
	size_t i;
	int h;
	int findtime, cachetime;

	if (file && handle)
		Sys_Error ("COM_FindFile: both handle and file set");
	if (!file && !handle)
		Sys_Error ("COM_FindFile: neither handle or file set");
		
//
// search through the path, one element at a time
//
	search = com_searchpaths;

	for ( ; search ; search = search->next)
	{
	// is the element a pak file?
		if (search->pack)
		{
		// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
				if (!strcmp (pak->files[i].name, filename))
				{       // found it!
					Sys_Printf ("PackFile: %s : %s\n",pak->filename, filename);
					if (handle)
					{
						*handle = pak->handle;
						Sys_FileSeek (pak->handle, pak->files[i].filepos);
					}
					else
					{       // open a new file on the pakfile
						*file = fopen (pak->filename, "rb");
						if (*file)
							fseek (*file, pak->files[i].filepos, SEEK_SET);
					}
					com_filesize = pak->files[i].filelen;
					return com_filesize;
				}
		}
		else
		{               
	// check a file in the directory tree
#ifdef QUAKE_REGISTERED
			if (!static_registered)
			{       // if not a registered version, don't ever go beyond base
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}
#endif
			
			sprintf (netpath, "%s/%s",search->filename, filename);
			
			findtime = Sys_FileTime (netpath);
			if (findtime == -1)
				continue;
				
		// see if the file needs to be updated in the cache
			if (!com_cachedir[0])
				strcpy (cachepath, netpath);
			else
			{	
#ifdef QUAKE_WINDOWS
				if ((strlen(netpath) < 2) || (netpath[1] != ':'))
					sprintf (cachepath,"%s%s", com_cachedir, netpath);
				else
					sprintf (cachepath,"%s%s", com_cachedir, netpath+2);
#else
				sprintf (cachepath,"%s%s", com_cachedir, netpath);
#endif

				cachetime = Sys_FileTime (cachepath);
			
				if (cachetime < findtime)
					COM_CopyFile (netpath, cachepath);
				strcpy (netpath, cachepath);
			}	

			Sys_Printf ("FindFile: %s\n",netpath);
			com_filesize = Sys_FileOpenRead (netpath, &h);
			if (handle)
				*handle = h;
			else
			{
				Sys_FileClose (h);
				*file = fopen (netpath, "rb");
			}
			return com_filesize;
		}
		
	}
	
	Sys_Printf ("FindFile: can't find %s\n", filename);
	
	if (handle)
		*handle = -1;
	else
		*file = NULL;
	com_filesize = 0;
	return 0;
}


/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
size_t COM_OpenFile (char *filename, int *handle)
{
	return COM_FindFile (filename, handle, NULL);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
size_t COM_FOpenFile (char *filename, FILE **file)
{
	return COM_FindFile (filename, NULL, file);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile (int h)
{
	searchpath_t    *s;
	
	for (s = com_searchpaths ; s ; s=s->next)
		if (s->pack && s->pack->handle == h)
			return;
			
	Sys_FileClose (h);
}


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/

cache_user_t *loadcache;
byte *loadbuf;
size_t loadsize;

static byte *COM_LoadFile (char *path, int usehunk)
{
	int h;
	byte *buf;
	char base[32];
	size_t len;

	buf = NULL;     // quiet compiler warning

// look for it in the filesystem or pack files
	len = COM_OpenFile (path, &h);
	if (h == -1)
		return NULL;
	
// extract the filename base name for hunk tag
	COM_FileBase (path, base);
	
	if (usehunk == 1)
		buf = Hunk_AllocName (len+1, base);
	else if (usehunk == 2)
		buf = Hunk_TempAlloc (len+1);
	else if (usehunk == 0)
		buf = Z_Malloc (len+1);
	else if (usehunk == 3)
		buf = Cache_Alloc (loadcache, len+1, base);
	else if (usehunk == 4)
	{
		if (len+1 > loadsize)
			buf = Hunk_TempAlloc (len+1);
		else
			buf = loadbuf;
	}
	else
		Sys_Error ("COM_LoadFile: bad usehunk");

	if (!buf)
		Sys_Error ("COM_LoadFile: not enough space for %s", path);
		
	((byte *)buf)[len] = 0;

	Draw_BeginDisc ();
	Sys_FileRead (h, buf, len);                     
	COM_CloseFile (h);
	Draw_EndDisc ();

	return buf;
}

byte *COM_LoadHunkFile (char *path)
{
	return COM_LoadFile (path, 1);
}

byte *COM_LoadTempFile (char *path)
{
	return COM_LoadFile (path, 2);
}

void COM_LoadCacheFile (char *path, struct cache_user_s *cu)
{
	loadcache = cu;
	COM_LoadFile (path, 3);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (char *path, void *buffer, size_t bufsize)
{
	byte    *buf;
	
	loadbuf = (byte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, 4);
	
	return buf;
}

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
static pack_t *COM_LoadPackFile (char *packfile, bool *modified)
{
	dpackheader_t   header;
	size_t i;
	packfile_t *newfiles;
	size_t numpackfiles;
	pack_t *pack;
	int packhandle;
	dpackfile_t info[MAX_FILES_IN_PACK];
	unsigned short crc;

	if (Sys_FileOpenRead (packfile, &packhandle) == -1)
	{
//              Con_Printf ("Couldn't open %s\n", packfile);
		return NULL;
	}
	Sys_FileRead (packhandle, (void *)&header, sizeof(header));
	if (header.id[0] != 'P' || header.id[1] != 'A'
	|| header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error ("%s is not a packfile", packfile);
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error ("%s has %lu files", packfile, numpackfiles);

#ifdef QUAKE_REGISTERED
	if (modified && numpackfiles != PAK0_COUNT)
		*modified = true;    // not the original file
#endif

	newfiles = Hunk_AllocName (numpackfiles * sizeof(packfile_t), "packfile");

	Sys_FileSeek (packhandle, header.dirofs);
	Sys_FileRead (packhandle, (void *)info, header.dirlen);

// crc the directory to check for modifications
#ifdef QUAKE_REGISTERED
	if (modified)
	{
		CRC_Init (&crc);
		for (i=0 ; i<header.dirlen ; i++)
			CRC_ProcessByte (&crc, ((byte *)info)[i]);
		if (crc != PAK0_CRC)
			*modified = true;
	}
#endif

// parse the directory
	for (i=0 ; i<numpackfiles ; i++)
	{
		strcpy (newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	pack = Hunk_Alloc (sizeof (pack_t));
	strcpy (pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	
	Con_Printf ("Added packfile %s (%lu files)\n", packfile, numpackfiles);
	return pack;
}


/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ... 
================
*/
static void COM_AddGameDirectory (char *dir, bool *modified)
{
	int                             i;
	searchpath_t    *search;
	pack_t                  *pak;
	char                    pakfile[MAX_OSPATH];

	strcpy (com_gamedir, dir);

//
// add the directory to the search path
//
	search = Hunk_Alloc (sizeof(searchpath_t));
	strcpy (search->filename, dir);
	search->next = com_searchpaths;
	com_searchpaths = search;

//
// add any pak files in the format pak0.pak pak1.pak, ...
//
	for (i=0 ; ; i++)
	{
		sprintf (pakfile, "%s/pak%i.pak", dir, i);
		pak = COM_LoadPackFile (pakfile, modified);
		if (!pak)
			break;
		search = Hunk_Alloc (sizeof(searchpath_t));
		search->pack = pak;
		search->next = com_searchpaths;
		com_searchpaths = search;               
	}
}

/*
================
COM_InitFilesystem
================
*/
static void COM_InitFilesystem ()
{
	int             i, j;
	char    basedir[MAX_OSPATH];
	searchpath_t    *search;
	bool modified = false;

//
// -basedir <path>
// Overrides the system supplied base directory (under QUAKE_BASEDIR)
//
	i = COM_CheckParm ("-basedir");
	if (i && i < com_argc-1)
		strcpy (basedir, com_argv[i+1]);
	else
		strcpy (basedir, host_parms.basedir);

	j = strlen (basedir);

	if (j > 0)
	{
		if ((basedir[j-1] == '\\') || (basedir[j-1] == '/'))
			basedir[j-1] = 0;
	}

//
// -cachedir <path>
// Overrides the system supplied cache directory (NULL or /qcache)
// -cachedir - will disable caching.
//
	i = COM_CheckParm ("-cachedir");
	if (i && i < com_argc-1)
	{
		if (com_argv[i+1][0] == '-')
			com_cachedir[0] = 0;
		else
			strcpy (com_cachedir, com_argv[i+1]);
	}
	else if (host_parms.cachedir)
		strcpy (com_cachedir, host_parms.cachedir);
	else
		com_cachedir[0] = 0;

//
// start up with QUAKE_BASEDIR by default
//
	COM_AddGameDirectory (va("%s/"QUAKE_BASEDIR, basedir), &modified);

	if (COM_CheckParm ("-rogue"))
	{
		modified = true;
		COM_AddGameDirectory (va("%s/rogue", basedir), NULL);
	}
	if (COM_CheckParm ("-hipnotic"))
	{
		modified = true;
		COM_AddGameDirectory (va("%s/hipnotic", basedir), NULL);
	}

//
// -game <gamedir>
// Adds basedir/gamedir as an override game
//
	i = COM_CheckParm ("-game");
	if (i && i < com_argc-1)
	{
		modified = true;
		COM_AddGameDirectory (va("%s/%s", basedir, com_argv[i+1]), NULL);
	}

//
// -path <dir or packfile> [<dir or packfile>] ...
// Fully specifies the exact serach path, overriding the generated one
//
	i = COM_CheckParm ("-path");
	if (i)
	{
		modified = true;
		com_searchpaths = NULL;
		while (++i < com_argc)
		{
			if (!com_argv[i] || com_argv[i][0] == '+' || com_argv[i][0] == '-')
				break;
			
			search = Hunk_Alloc (sizeof(searchpath_t));
			if ( !strcmp(COM_FileExtension(com_argv[i]), "pak") )
			{
				search->pack = COM_LoadPackFile (com_argv[i], NULL);
				if (!search->pack)
					Sys_Error ("Couldn't load packfile: %s", com_argv[i]);
			}
			else
				strcpy (search->filename, com_argv[i]);
			search->next = com_searchpaths;
			com_searchpaths = search;
		}
	}

#ifdef QUAKE_REGISTERED
	COM_CheckRegistered (modified);
#endif
}


