
#ifndef _PR_COMP_H
#define _PR_COMP_H

typedef int32_t func_t;
typedef int32_t string_t;

#define PROG_VERSION 6

typedef enum
{
	ev_void,
	ev_string,
	ev_float,
	ev_vector,
	ev_entity,
	ev_field,
	ev_function,
	ev_pointer,
	ev_types,
} etype_t;

enum
{
	OFS_NULL = 0,
	OFS_RETURN = 1,
	OFS_PARM0 = 4, // leave 3 ofs for each parm to hold vectors
	OFS_PARM1 = 7,
	OFS_PARM2 = 10,
	OFS_PARM3 = 13,
	OFS_PARM4 = 16,
	OFS_PARM5 = 19,
	OFS_PARM6 = 22,
	OFS_PARM7 = 25,
	RESERVED_OFS = 28,
};

enum
{
	OP_DONE,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_LOAD_F,
	OP_LOAD_V,
	OP_LOAD_S,
	OP_LOAD_ENT,
	OP_LOAD_FLD,
	OP_LOAD_FNC,

	OP_ADDRESS,

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_FLD,
	OP_STORE_FNC,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,
	OP_STOREP_FLD,
	OP_STOREP_FNC,

	OP_RETURN,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,
	OP_NOT_FNC,
	OP_IF,
	OP_IFNOT,
	OP_CALL0,
	OP_CALL1,
	OP_CALL2,
	OP_CALL3,
	OP_CALL4,
	OP_CALL5,
	OP_CALL6,
	OP_CALL7,
	OP_CALL8,
	OP_STATE,
	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR
};

typedef struct statement_s
{
	uint16_t op;
	int16_t a, b, c;
} dstatement_t;

typedef struct
{
	uint16_t type; // if DEF_SAVEGLOBAL bit is set
						 // the variable needs to be saved in savegames
	uint16_t ofs;
	int32_t s_name;
} ddef_t;
#define DEF_SAVEGLOBAL (1 << 15)

#define MAX_PARMS 8

typedef struct
{
	int32_t first_statement; // negative numbers are builtins
	int32_t parm_start;
	int32_t locals; // total ints of parms + locals

	int32_t profile; // runtime

	int32_t s_name;
	int32_t s_file; // source file defined in

	int32_t numparms;
	byte parm_size[MAX_PARMS];
} dfunction_t;

typedef struct
{
	int32_t version;
	int32_t crc; // check of header file

	int32_t ofs_statements;
	int32_t numstatements; // statement 0 is an error

	int32_t ofs_globaldefs;
	int32_t numglobaldefs;

	int32_t ofs_fielddefs;
	int32_t numfielddefs;

	int32_t ofs_functions;
	int32_t numfunctions; // function 0 is an empty

	int32_t ofs_strings;
	int32_t numstrings; // first string is a null string

	int32_t ofs_globals;
	int32_t numglobals;

	int32_t entityfields;
} dprograms_t;

#endif /* !_PR_COMP_H */
