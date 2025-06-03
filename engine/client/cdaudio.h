
#ifndef _CDAUDIO_H
#define _CDAUDIO_H

int CDAudio_Init (void);
void CDAudio_Play (byte track, bool looping);
void CDAudio_Stop (void);
void CDAudio_Pause (void);
void CDAudio_Resume (void);
void CDAudio_Shutdown (void);
void CDAudio_Update (void);
void CDAudio_Paint (void *out, int idx, int count);

#endif /* !_CDAUDIO_H */
