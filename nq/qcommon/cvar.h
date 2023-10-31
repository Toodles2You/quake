#ifndef _CVAR_H
#define _CVAR_H

enum
{
    CVAR_ARCHIVE 	= 1,  // set to cause it to be saved to vars.rc
    CVAR_SERVER     = 2,  // notifies players when changed
};

typedef struct cvar_s
{
	char *name;
	char *string;
	unsigned int flags;
	float value;
	struct cvar_s *next;
} cvar_t;

void Cvar_RegisterVariable(cvar_t *variable);
// registers a cvar that allready has the name, string, and optionally the
// archive elements set.

void Cvar_Set(char *var_name, char *value);
// equivelant to "<name> <variable>" typed at the console

void Cvar_SetValue(char *var_name, float value);
// expands value to a string and calls Cvar_Set

float Cvar_VariableValue(char *var_name);
// returns 0 if not defined or non numeric

char *Cvar_VariableString(char *var_name);
// returns an empty string if not defined

char *Cvar_CompleteVariable(char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

bool Cvar_Command();
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables(FILE *f);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar(char *var_name);

extern cvar_t *cvar_vars;

#endif /* !_CVAR_H */
