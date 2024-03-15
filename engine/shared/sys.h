
#ifndef _SYS_H
#define _SYS_H

//
// file IO
//

// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
off_t Sys_FileOpenRead(char *path, int *hndl);

int Sys_FileOpenWrite(char *path);
void Sys_FileClose(int handle);
void Sys_FileSeek(int handle, size_t position);
ssize_t Sys_FileRead(int handle, void *dest, size_t count);
ssize_t Sys_FileWrite(int handle, void *data, size_t count);
int Sys_FileTime(char *path);
void Sys_mkdir(char *path);

//
// system IO
//
void Sys_DebugLog(char *file, char *fmt, ...);

void Sys_Error(char *error, ...);
// an error will cause the entire program to exit

void Sys_Printf(char *fmt, ...);
// send text to the console

void Sys_Quit();

double Sys_FloatTime();

char *Sys_ConsoleInput();

void Sys_SendKeyEvents();
// Perform Key_Event () callbacks until the input que is empty

#endif /* !_SYS_H */
