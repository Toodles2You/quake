
// the status bar is only redrawn if something has changed, but if anything
// does, the entire thing will be redrawn for the next vid.numpages frames.

#ifndef _SBAR_H
#define _SBAR_H

#define SBAR_HEIGHT 24

extern int sb_lines; // scan lines to draw

void Sbar_Init();

void Sbar_Changed();
// call whenever any of the client stats represented on the sbar changes

void Sbar_Draw();
// called every frame by screen

void Sbar_IntermissionOverlay();
// called each frame after the level has been completed

void Sbar_FinaleOverlay();

#endif /* !_SBAR_H */
