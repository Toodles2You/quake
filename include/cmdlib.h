// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef unsigned char byte;

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


// set these before calling CheckParm
extern int myargc;
extern char **myargv;

char *strupr (char *in);
char *strlower (char *in);

#ifdef QUAKE_WINDOWS
int strncasecmp (char *s1, char *s2, int n);
int strcasecmp (char *s1, char *s2);
#endif

void Q_getwd (char *out);

int filelength (FILE *f);
int	FileTime (char *path);

void	Q_mkdir (char *path);

extern	char		qdir[1024];
extern	char		gamedir[1024];
void SetQdirFromPath (char *path);
char *ExpandPath (char *path);
char *ExpandPathAndArchive (char *path);


double I_FloatTime (void);

void	Error (char *error, ...);
int		CheckParm (char *check);

FILE	*SafeOpenWrite (char *filename);
FILE	*SafeOpenRead (char *filename);
void	SafeRead (FILE *f, void *buffer, int count);
void	SafeWrite (FILE *f, void *buffer, int count);

int		LoadFile (char *filename, void **bufferptr);
void	SaveFile (char *filename, void *buffer, int count);

void 	DefaultExtension (char *path, char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase (char *path, char *dest, bool ext);
void	ExtractFileExtension (char *path, char *dest);

int 	ParseNum (char *str);

int16_t	BigShort (int16_t l);
int16_t	LittleShort (int16_t l);
int32_t BigLong (int32_t l);
int32_t LittleLong (int32_t l);
float	BigFloat (float l);
float	LittleFloat (float l);


char *COM_Parse (char *data);

extern	char		com_token[1024];
extern	bool	com_eof;

char *copystring(char *s);


void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);

void	CreatePath (char *path);
void CopyFile (char *from, char *to);

extern	bool		archive;
extern	char			archivedir[1024];


#endif
