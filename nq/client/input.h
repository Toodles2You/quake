
#ifndef _INPUT_H
#define _INPUT_H

void IN_Init();

void IN_Shutdown();

void IN_Commands();
// oportunity for devices to stick commands on the script buffer

void IN_Move(usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd

#endif /* !_INPUT_H */
