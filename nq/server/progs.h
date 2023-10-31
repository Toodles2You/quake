
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

	// C exported fields from progs
	// other fields from progs come immediately after
} edict_t;

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l, edict_t, area)

typedef struct progs_state_s progs_state_t;

typedef void (*builtin_t)(progs_state_t *);

#define PR_FIELD(_, name) pr_##name,
#define PR_FIELD_OPTIONAL(_, name) pr_##name,

typedef enum {
	#include "pr_globals.h"
	pr_globals_count
} progs_globals_e;

typedef enum {
	#include "pr_fields.h"
	pr_fields_count
} progs_fields_e;

#undef PR_FIELD
#undef PR_FIELD_OPTIONAL

typedef struct {
	char *name;
	bool optional;
} pr_field_t;

typedef struct progs_state_s {
	dprograms_t *progs;
	dfunction_t *functions;
	char *strings;
	ddef_t *globaldefs;
	ddef_t *fielddefs;
	dstatement_t *statements;
	float *globals;

	const uint32_t *global_struct;
	const uint32_t *field_struct;
	const size_t edict_size;
	
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
void PR_BuildStructs(progs_state_t *pr, uint32_t *globalStruct, pr_field_t *globalFields, uint32_t *fieldStruct, pr_field_t *fields);

ddef_t *PR_GlobalAtOfs (progs_state_t *pr, int ofs);
char *PR_GlobalString (progs_state_t *pr, int ofs);
char *PR_GlobalStringNoContents (progs_state_t *pr, int ofs);
ddef_t *PR_FieldAtOfs (progs_state_t *pr, int ofs);
ddef_t *PR_FindField (progs_state_t *pr, char *name);
ddef_t *PR_FindGlobal (progs_state_t *pr, char *name);
dfunction_t *PR_FindFunction (progs_state_t *pr, char *name);
char *PR_ValueString(progs_state_t *pr, etype_t type, eval_t *val);
char *PR_UglyValueString(progs_state_t *pr, etype_t type, eval_t *val);

void PR_Profile_f();

void ED_ClearEdict (edict_t *e);
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

void ED_Init();

edict_t *EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t *e);

#define NEXT_EDICT(e) ((edict_t *)((byte *)e + sv.pr.edict_size))

#define EDICT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t *)((byte *)sv.edicts + e))

//============================================================================

#define pr_global(_PR, _TYPE, _OFS) (*(_TYPE *)(_PR->globals + _OFS))
#define pr_global_ptr(_PR, _TYPE, _OFS) ((_TYPE *)(_PR->globals + _OFS))
#define pr_float(_PR, _FIELD) (*(float *)(_PR->globals + _PR->global_struct[pr_##_FIELD]))
#define pr_int(_PR, _FIELD) (*(int32_t *)(_PR->globals + _PR->global_struct[pr_##_FIELD]))
#define pr_vector(_PR, _FIELD) ((float *)(_PR->globals + _PR->global_struct[pr_##_FIELD]))

#define sv_pr_float(_FIELD) (*(float *)(sv.pr.globals + sv.pr.global_struct[pr_##_FIELD]))
#define sv_pr_int(_FIELD) (*(int32_t *)(sv.pr.globals + sv.pr.global_struct[pr_##_FIELD]))
#define sv_pr_vector(_FIELD) ((float *)(sv.pr.globals + sv.pr.global_struct[pr_##_FIELD]))
#define sv_pr_execute(_FIELD) PR_ExecuteProgram (&sv.pr, sv.pr.global_struct[pr_##_FIELD])

#define pr_get_string(_PR, _OFS) PR_GetString(_PR, pr_global(_PR, string_t, _OFS))
#define pr_set_string(_PR, _OFS, _STR) (pr_global(_PR, string_t, _OFS) = PR_SetString(_PR, _STR))

#define pr_get_edict(_PR, _OFS) ((edict_t *)((byte *)sv.edicts + pr_global(_PR, int32_t, _OFS)))
#define pr_get_edict_num(_PR, _OFS) NUM_FOR_EDICT(pr_get_edict(_PR, _OFS))

#define pr_field(_FIELD) (sv.pr.global_struct[pr_##_FIELD] != 0)

#define ed_float(_ED, _FIELD) (*(float *)((float *)(_ED + 1) + sv.pr.field_struct[pr_##_FIELD]))
#define ed_int(_ED, _FIELD) (*(int32_t *)((float *)(_ED + 1) + sv.pr.field_struct[pr_##_FIELD]))
#define ed_vector(_ED, _FIELD) ((float *)((float *)(_ED + 1) + sv.pr.field_struct[pr_##_FIELD]))

#define ed_get_string(_ED, _FIELD) PR_GetString(&sv.pr, ed_int(_ED, _FIELD))
#define ed_set_string(_ED, _FIELD, _STR) (ed_int(_ED, _FIELD) = PR_SetString(&sv.pr, _STR))

#define ed_get_edict(_ED, _FIELD) PROG_TO_EDICT(ed_int(_ED, _FIELD))
#define ed_set_edict(_ED, _FIELD, _ED2) (ed_int(_ED, _FIELD) = EDICT_TO_PROG(_ED2))

#define ed_field(_FIELD) (sv.pr.field_struct[pr_##_FIELD] != 0)

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
