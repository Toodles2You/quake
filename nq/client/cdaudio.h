
#ifndef _CDAUDIO_H
#define _CDAUDIO_H

int CDAudio_Init();
void CDAudio_Play(byte track, bool looping);
void CDAudio_Stop();
void CDAudio_Pause();
void CDAudio_Resume();
void CDAudio_Shutdown();
void CDAudio_Update();
void CDAudio_Paint(void *out, int idx, int count);

#endif /* !_CDAUDIO_H */
