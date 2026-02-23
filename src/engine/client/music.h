
#ifndef _MUSIC_H
#define _MUSIC_H

void Music_Stop (void);
void Music_Play (int track, bool looping);
void Music_Pause (void);
void Music_Resume (void);
void Music_Update (void);
void Music_Init (void);
void Music_Shutdown (void);

extern cvar_t bgmvolume;

#endif /* !_MUSIC_H */
