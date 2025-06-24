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

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#include "clientdef.h"
#include "serverdef.h"

static bool nostdout;

void Sys_Printf (char *fmt, ...)
{
	va_list argptr;
	char text[1024];
	unsigned char *p;

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	if (strlen (text) > sizeof (text))
		Sys_Error ("memory overwrite in Sys_Printf");

	if (nostdout)
		return;

	for (p = (unsigned char *)text; *p; p++)
	{
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf ("[%02x]", *p);
		else
			putc (*p, stdout);
	}
}

void Sys_Quit (void)
{
	Host_Shutdown ();
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	fflush (stdout);
	exit (0);
}

void Sys_Init (void) {}

void Sys_Error (char *error, ...)
{
	va_list argptr;
	char string[1024];

	// change stdin to non blocking
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	va_start (argptr, error);
	vsprintf (string, error, argptr);
	va_end (argptr);
	fprintf (stderr, "Error: %s\n", string);

	Host_Shutdown ();
	exit (1);
}

void Sys_Warn (char *warning, ...)
{
	va_list argptr;
	char string[1024];

	va_start (argptr, warning);
	vsprintf (string, warning, argptr);
	va_end (argptr);
	fprintf (stderr, "Warning: %s", string);
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime (char *path)
{
	struct stat buf;

	if (stat (path, &buf) == -1)
		return -1;

	return buf.st_mtime;
}

void Sys_mkdir (char *path)
{
	mkdir (path, 0777);
}

off_t Sys_FileOpenRead (char *path, int *handle)
{
	int h;
	struct stat fileinfo;

	h = open (path, O_RDONLY, 0666);
	*handle = h;
	if (h == -1)
		return -1;

	if (fstat (h, &fileinfo) == -1)
		Sys_Error ("Error fstating %s", path);

	return fileinfo.st_size;
}

int Sys_FileOpenWrite (char *path)
{
	int handle;

	umask (0);

	handle = open (path, O_RDWR | O_CREAT | O_TRUNC, 0666);

	if (handle == -1)
		Sys_Error ("Error opening %s: %s", path, strerror (errno));

	return handle;
}

ssize_t Sys_FileWrite (int handle, void *src, size_t count)
{
	return write (handle, src, count);
}

void Sys_FileClose (int handle)
{
	close (handle);
}

void Sys_FileSeek (int handle, size_t position)
{
	lseek (handle, position, SEEK_SET);
}

ssize_t Sys_FileRead (int handle, void *dest, size_t count)
{
	return read (handle, dest, count);
}

void Sys_DebugLog (char *file, char *fmt, ...)
{
	va_list argptr;
	static char data[1024];
	int fd;

	va_start (argptr, fmt);
	vsprintf (data, fmt, argptr);
	va_end (argptr);
	//    fd = open(file, O_WRONLY | O_BINARY | O_CREAT | O_APPEND, 0666);
	fd = open (file, O_WRONLY | O_CREAT | O_APPEND, 0666);
	write (fd, data, strlen (data));
	close (fd);
}

void Sys_EditFile (char *filename)
{
	char cmd[256];
	char *term;
	char *editor;

	term = getenv ("TERM");
	if (term && !strcmp (term, "xterm"))
	{
		editor = getenv ("VISUAL");
		if (!editor)
			editor = getenv ("EDITOR");
		if (!editor)
			editor = getenv ("EDIT");
		if (!editor)
			editor = "vi";
		sprintf (cmd, "xterm -e %s %s", editor, filename);
		system (cmd);
	}
}

double Sys_FloatTime (void)
{
	struct timeval tp;
	struct timezone tzp;
	static int secbase;

	gettimeofday (&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}

void Sys_Sleep (int msec)
{
	struct timespec ts;
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	nanosleep (&ts, NULL);
}

char *Sys_ConsoleInput (void)
{
	static char text[256];
	int len;
	fd_set fdset;
	struct timeval timeout;

	if (cls.state == ca_dedicated)
	{
		FD_ZERO (&fdset);
		FD_SET (0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET (0, &fdset))
			return NULL;

		len = read (0, text, sizeof (text));
		if (len < 1)
			return NULL;
		text[len - 1] = 0; // rip off the /n and terminate

		return text;
	}
	return NULL;
}

int main (int c, char **v)
{
	double time, oldtime, newtime;
	quakeparms_t parms;
	int j;

	signal (SIGFPE, SIG_IGN);

	memset (&parms, 0, sizeof (parms));

	COM_InitArgv (c, v);
	parms.argc = com_argc;
	parms.argv = com_argv;

	parms.memsize = 16 * 1024 * 1024;

	j = COM_CheckParm ("-mem");
	if (j)
		parms.memsize = (size_t)atoi (com_argv[j + 1]) * 1024 * 1024;
	parms.membase = malloc (parms.memsize);

	parms.basedir = ".";

	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	Host_Init (&parms);

	Sys_Init ();

	if (COM_CheckParm ("-nostdout"))
		nostdout = 1;
	else
	{
		fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
		printf ("Linux Quake -- Version " QUAKE_VERSION "\n");
	}

	oldtime = Sys_FloatTime () - 0.1;
	while (1)
	{
		// find time spent rendering last frame
		newtime = Sys_FloatTime ();
		time = newtime - oldtime;

		if (cls.state == ca_dedicated)
		{
			if (time < sys_ticrate.value)
			{
				usleep (1);
				continue; // not time to run a server only tic yet
			}
			time = sys_ticrate.value;
		}
		else if (!cls.timedemo)
		{
			double fps = cl_maxfps.value;
			if (fps < 30.0)
				fps = 30.0;
			fps = 1.0 / fps;

			if (!cls.timedemo && time < fps)
			{
				Sys_Sleep ((fps - time) * 1000); // framerate is too high
				continue;
			}
		}

		if (cls.state == ca_dedicated && time > sys_ticrate.value * 2)
			oldtime = newtime;
		else
			oldtime += time;

		Host_Frame (time);
	}
}
