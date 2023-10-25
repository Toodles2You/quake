
#ifndef _PROGS_H
#define _PROGS_H

#include "pr_comp.h"  // defs shared with qcc
#include "progdefs.h" // generated by program cdefs

typedef union eval_s
{
	string_t string;
	float _float;
	float vector[3];
	func_t function;
	int _int;
	int edict;
} eval_t;

#define MAX_ENT_LEAFS 16

typedef struct edict_s
{
	bool free;
	link_t area; // linked to a division node or leaf

	int num_leafs;
	short leafnums[MAX_ENT_LEAFS];

	entity_state_t baseline;

	float freetime; // sv.time when the object was freed
	entvars_t v;	// C exported fields from progs
	// other fields from progs come immediately after
} edict_t;

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l, edict_t, area)

typedef struct progs_state_s progs_state_t;

typedef void (*builtin_t)(progs_state_t *);

typedef struct progs_state_s {
	dprograms_t *progs;
	dfunction_t *functions;
	char *strings;
	ddef_t *globaldefs;
	ddef_t *fielddefs;
	dstatement_t *statements;
	globalvars_t *global_struct;
	float *globals;

	size_t edict_size;
	
	builtin_t *builtins;
	size_t numbuiltins;
	
	size_t argc;

	dfunction_t	*xfunction;
	int xstatement;

	unsigned short crc;
} progs_state_t;

//============================================================================

void PR_Init();

void PR_ExecuteProgram(progs_state_t *pr, func_t fnum);
int PR_LoadProgs(progs_state_t *pr, char *filename, int version, int crc);

ddef_t *PR_GlobalAtOfs (progs_state_t *pr, int ofs);
char *PR_GlobalString (progs_state_t *pr, int ofs);
char *PR_GlobalStringNoContents (progs_state_t *pr, int ofs);
ddef_t *PR_FieldAtOfs (progs_state_t *pr, int ofs);
ddef_t *PR_FindField (progs_state_t *pr, char *name);
ddef_t *PR_FindGlobal (progs_state_t *pr, char *name);
dfunction_t *PR_FindFunction (progs_state_t *pr, char *name);

void PR_Profile_f();

edict_t *ED_Alloc();
void ED_Free(edict_t *ed);

char *ED_NewString(char *string);
// returns a copy of the string allocated from the server's string heap

void ED_Print(edict_t *ed);
void ED_Write(FILE *f, edict_t *ed);
char *ED_ParseEdict(char *data, edict_t *ent);

void ED_WriteGlobals(FILE *f);
void ED_ParseGlobals(char *data);

void ED_LoadFromFile(char *data);

edict_t *EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t *e);

#define NEXT_EDICT(e) ((edict_t *)((byte *)e + sv.pr.edict_size))

#define EDICT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t *)((byte *)sv.edicts + e))

//============================================================================

#define G_FLOAT(o) (pr->globals[o])
#define G_INT(o) (*(int32_t *)&pr->globals[o])
#define G_EDICT(o) ((edict_t *)((byte *)sv.edicts + *(int32_t *)&sv.pr.globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define G_VECTOR(o) (&pr->globals[o])
#define G_STRING(o) (PR_GetString(pr, *(string_t *)&pr->globals[o]))
#define G_FUNCTION(o) (*(func_t *)&pr->globals[o])

#define E_FLOAT(e, o) (((float *)&e->v)[o])
#define E_INT(e, o) (*(int32_t *)&((float *)&e->v)[o])
#define E_VECTOR(e, o) (&((float *)&e->v)[o])
#define E_STRING(e, o) (PR_GetString(pr, *(string_t *)&((float *)&e->v)[o]))

extern int pr_type_size[ev_types];

extern builtin_t *pr_builtins;
extern int pr_numbuiltins;

extern bool pr_trace;

void PR_RunError(progs_state_t *pr, char *error, ...);

void ED_PrintEdicts();
void ED_PrintNum(int ent);

eval_t *GetEdictFieldValue(edict_t *ed, char *field);

//
// PR STrings stuff
//
#define MAX_PRSTR 1024

extern char *pr_strtbl[MAX_PRSTR];
extern int num_prstr;

char *PR_GetString(progs_state_t *pr, int num);
int PR_SetString(progs_state_t *pr, char *s);
void PR_CheckEmptyString (progs_state_t *pr, char *s);

//============================================================================

void PF_changeyaw (progs_state_t *pr);

#endif /* !_PROGS_H */
