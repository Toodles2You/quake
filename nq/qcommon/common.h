
#ifndef _COMMON_H
#define _COMMON_H

typedef unsigned char byte;

#define MAX_INFO_STRING 196
#define MAX_SERVERINFO_STRING 512
#define MAX_LOCALINFO_STRING 32768

#define lengthof(array) (sizeof(array) / sizeof(array[0]))

//============================================================================

typedef struct sizebuf_s
{
	bool allowoverflow; // if false, do a Sys_Error
	bool overflowed;	// set to true if the buffer size failed
	byte *data;
	int maxsize;
	int cursize;
} sizebuf_t;

void SZ_Alloc(sizebuf_t *buf, int startsize);
void SZ_Free(sizebuf_t *buf);
void SZ_Clear(sizebuf_t *buf);
void *SZ_GetSpace(sizebuf_t *buf, int length);
void SZ_Write(sizebuf_t *buf, void *data, int length);
void SZ_Print(sizebuf_t *buf, char *data); // strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s *prev, *next;
} link_t;

void ClearLink(link_t *l);
void RemoveLink(link_t *l);
void InsertLinkBefore(link_t *l, link_t *before);
void InsertLinkAfter(link_t *l, link_t *after);

#ifndef QUAKE_LINUX
#define offsetof(type, member) ((uintptr_t)&(((type *)0)->member))
#endif

#define STRUCT_FROM_LINK(l, t, m) ((t *)((byte *)l - offsetof(t, m)))

//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((int16_t)0x7fff)
#define Q_MAXINT ((int32_t)0x7fffffff)
#define Q_MAXLONG ((int32_t)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((int16_t)0x8000)
#define Q_MININT ((int32_t)0x80000000)
#define Q_MINLONG ((int32_t)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern bool bigendien;

extern int16_t (*BigShort)(int16_t l);
extern int16_t (*LittleShort)(int16_t l);
extern int32_t (*BigLong)(int32_t l);
extern int32_t (*LittleLong)(int32_t l);
extern float (*BigFloat)(float l);
extern float (*LittleFloat)(float l);

//============================================================================

struct usercmd_s;
extern struct usercmd_s nullcmd;

void MSG_WriteChar(sizebuf_t *sb, int c);
void MSG_WriteByte(sizebuf_t *sb, int c);
void MSG_WriteShort(sizebuf_t *sb, int c);
void MSG_WriteLong(sizebuf_t *sb, int c);
void MSG_WriteFloat(sizebuf_t *sb, float f);
void MSG_WriteString(sizebuf_t *sb, char *s);
void MSG_WriteCoord(sizebuf_t *sb, float f);
void MSG_WriteAngle(sizebuf_t *sb, float f);
void MSG_WriteAngle16(sizebuf_t *sb, float f);
void MSG_WriteDeltaUsercmd(sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);

extern int msg_readcount;
extern bool msg_badread; // set if a read goes beyond end of message

void MSG_BeginReading();
int MSG_GetReadCount();
int MSG_ReadChar();
int MSG_ReadByte();
int MSG_ReadShort();
int MSG_ReadLong();
float MSG_ReadFloat();
char *MSG_ReadString();
char *MSG_ReadStringLine();

float MSG_ReadCoord();
float MSG_ReadAngle();
float MSG_ReadAngle16();
void MSG_ReadDeltaUsercmd(struct usercmd_s *from, struct usercmd_s *cmd);

//============================================================================

#ifdef QUAKE_WINDOWS
int strcasecmp(char *s1, char *s2);
int strncasecmp(char *s1, char *s2, int n);
#endif

//============================================================================

extern char com_token[1024];
extern bool com_eof;

char *COM_Parse(char *data);
bool COM_BeginReadPairs(byte **data);
bool COM_ReadPair(byte **data, char *key, char *value);
extern int com_argc;
extern char **com_argv;

int COM_CheckParm(char *parm);
void COM_AddParm(char *parm);

void COM_Init(char *path);
void COM_InitArgv(int argc, char **argv);

char *COM_SkipPath(char *pathname);
void COM_StripExtension(char *in, char *out);
void COM_FileBase(char *in, char *out);
void COM_DefaultExtension(char *path, char *extension);

char *va(char *format, ...);
// does a varargs printf into a temp buffer

//============================================================================

extern size_t com_filesize;
struct cache_user_s;

extern char com_gamedir[MAX_OSPATH];

void COM_WriteFile(char *filename, void *data, size_t len);
size_t COM_OpenFile(char *filename, int *hndl);
size_t COM_FOpenFile(char *filename, FILE **file);
void COM_CloseFile(int h);

byte *COM_LoadStackFile(char *path, void *buffer, size_t bufsize);
byte *COM_LoadTempFile(char *path);
byte *COM_LoadHunkFile(char *path);
void COM_LoadCacheFile(char *path, struct cache_user_s *cu);
void COM_CreatePath(char *path);
void COM_Gamedir(char *dir);

extern bool standard_quake;
extern bool rogue;
extern bool hipnotic;

char *Info_ValueForKey(char *s, char *key);
void Info_RemoveKey(char *s, char *key);
void Info_RemovePrefixedKeys(char *start, char prefix);
void Info_SetValueForKey(char *s, char *key, char *value, int maxsize);
void Info_SetValueForStarKey(char *s, char *key, char *value, int maxsize);
void Info_Print(char *s);

unsigned int COM_BlockChecksum(void *buffer, int length);
void COM_BlockFullChecksum(void *buffer, int len, unsigned char *outbuf);
byte COM_BlockSequenceCRCByte(byte *base, int length, int sequence);

#endif /* !COMMON_H */
