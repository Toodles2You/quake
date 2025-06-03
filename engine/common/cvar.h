#ifndef _CVAR_H
#define _CVAR_H

enum
{
	CVAR_ARCHIVE = 1, // set to cause it to be saved to vars.rc
	CVAR_NOTIFY = 2,  // notifies players when changed
	CVAR_CLIENT_INFO = 4,
	CVAR_SERVER_INFO = 8,
};

typedef struct cvar_s
{
	char *name;
	char *string;
	unsigned int flags;
	float value;
	struct cvar_s *next[2];
} cvar_t;

void Cvar_RegisterVariable (cmd_source_e src, cvar_t *variable);
// registers a cvar that allready has the name, string, and optionally the
// archive elements set.

void Cvar_Set (cmd_source_e src, char *var_name, char *value);
// equivelant to "<name> <variable>" typed at the console

void Cvar_SetValue (cmd_source_e src, char *var_name, float value);
// expands value to a string and calls Cvar_Set

float Cvar_VariableValue (cmd_source_e src, char *var_name);
// returns 0 if not defined or non numeric

char *Cvar_VariableString (cmd_source_e src, char *var_name);
// returns an empty string if not defined

char *Cvar_CompleteVariable (cmd_source_e src, char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

bool Cvar_Command (cmd_source_e src);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables (FILE *f);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar (cmd_source_e src, char *var_name);

#endif /* !_CVAR_H */
