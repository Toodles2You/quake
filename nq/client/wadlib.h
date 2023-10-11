
#ifndef _WADLIB_H
#define _WADLIB_H

void W_LoadWadFile(char *filename);
void W_CleanupName(char *in, char *out);
lumpinfo_t *W_GetLumpinfo(char *name);
void *W_GetLumpName(char *name);
void *W_GetLumpNum(int num);
void W_SwapPic(qpic_t *pic);

void W_LoadMapWadFile(char *filename);
bool W_LoadMapMiptexInfo(char *name, void *dst);
bool W_LoadMapMiptexData(void *dst);
void W_FreeMapWadFiles();

#endif /* !_WADLIB_H */
