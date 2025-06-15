
#ifndef _VIEW_H
#define _VIEW_H

extern cvar_t v_gamma;

extern float v_blend[4];

void V_Init (void);
void V_RenderView (void);
float V_CalcRoll (vec3_t angles, vec3_t velocity);
void V_UpdatePalette (void);
void V_ClampViewAngles (void);

#endif /* !_VIEW_H */
