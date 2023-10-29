/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023 Justin Keller

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
===========================================================================
*/

#include "serverdef.h"


ddef_t *PR_GlobalAtOfs(progs_state_t *pr, int ofs)
{
    ddef_t *def;
    int i;

    for (i = 0; i < pr->progs->numglobaldefs; i++)
    {
        def = &pr->globaldefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}


ddef_t *PR_FieldAtOfs(progs_state_t *pr, int ofs)
{
    ddef_t *def;
    int i;

    for (i = 0; i < pr->progs->numfielddefs; i++)
    {
        def = &pr->fielddefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}


ddef_t *PR_FindField(progs_state_t *pr, char *name)
{
    ddef_t *def;
    int i;

    for (i = 0; i < pr->progs->numfielddefs; i++)
    {
        def = &pr->fielddefs[i];
        if (!strcmp(PR_GetString(pr, def->s_name), name))
            return def;
    }
    return NULL;
}


ddef_t *PR_FindGlobal(progs_state_t *pr, char *name)
{
    ddef_t *def;
    int i;

    for (i = 0; i < pr->progs->numglobaldefs; i++)
    {
        def = &pr->globaldefs[i];
        if (!strcmp(PR_GetString(pr, def->s_name), name))
            return def;
    }
    return NULL;
}


dfunction_t *PR_FindFunction(progs_state_t *pr, char *name)
{
    dfunction_t *func;
    int i;

    for (i = 0; i < pr->progs->numfunctions; i++)
    {
        func = &pr->functions[i];
        if (!strcmp(PR_GetString(pr, func->s_name), name))
            return func;
    }
    return NULL;
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *PR_ValueString(progs_state_t *pr, etype_t type, eval_t *val)
{
    static char line[256];
    ddef_t *def;
    dfunction_t *f;

    type &= ~DEF_SAVEGLOBAL;

    switch (type)
    {
    case ev_string:
        sprintf(line, "%s", PR_GetString(pr, val->string));
        break;
#if 0
	case ev_entity:
		sprintf (line, "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)) );
		break;
#endif
    case ev_function:
        f = pr->functions + val->function;
        sprintf(line, "%s()", PR_GetString(pr, f->s_name));
        break;
    case ev_field:
        def = PR_FieldAtOfs(pr, val->_int);
        sprintf(line, ".%s", PR_GetString(pr, def->s_name));
        break;
    case ev_void:
        sprintf(line, "void");
        break;
    case ev_float:
        sprintf(line, "%5.1f", val->_float);
        break;
    case ev_vector:
        sprintf(line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
        break;
    case ev_pointer:
        sprintf(line, "pointer");
        break;
    default:
        sprintf(line, "bad type %i", type);
        break;
    }

    return line;
}


/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char *PR_UglyValueString(progs_state_t *pr, etype_t type, eval_t *val)
{
    static char line[256];
    ddef_t *def;
    dfunction_t *f;

    type &= ~DEF_SAVEGLOBAL;

    switch (type)
    {
    case ev_string:
        sprintf(line, "%s", PR_GetString(pr, val->string));
        break;
#if 0
	case ev_entity:
		sprintf (line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
#endif
    case ev_function:
        f = pr->functions + val->function;
        sprintf(line, "%s", PR_GetString(pr, f->s_name));
        break;
    case ev_field:
        def = PR_FieldAtOfs(pr, val->_int);
        sprintf(line, "%s", PR_GetString(pr, def->s_name));
        break;
    case ev_void:
        sprintf(line, "void");
        break;
    case ev_float:
        sprintf(line, "%f", val->_float);
        break;
    case ev_vector:
        sprintf(line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
        break;
    default:
        sprintf(line, "bad type %i", type);
        break;
    }

    return line;
}


/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char *PR_GlobalString(progs_state_t *pr, int ofs)
{
    char *s;
    int i;
    ddef_t *def;
    void *val;
    static char line[128];

    val = (void *)&pr->globals[ofs];
    def = PR_GlobalAtOfs(pr, ofs);
    if (!def)
    {
        sprintf(line, "%i(?)", ofs);
    }
    else
    {
        s = PR_ValueString(pr, def->type, val);
        sprintf(line, "%i(%s)%s", ofs, PR_GetString(pr, def->s_name), s);
    }

    i = strlen(line);
    for (; i < 20; i++)
    {
        strcat(line, " ");
    }
    strcat(line, " ");

    return line;
}


char *PR_GlobalStringNoContents(progs_state_t *pr, int ofs)
{
    int i;
    ddef_t *def;
    static char line[128];

    def = PR_GlobalAtOfs(pr, ofs);
    if (!def)
    {
        sprintf(line, "%i(?)", ofs);
    }
    else
    {
        sprintf(line, "%i(%s)", ofs, PR_GetString(pr, def->s_name));
    }

    i = strlen(line);
    for (; i < 20; i++)
    {
        strcat(line, " ");
    }
    strcat(line, " ");

    return line;
}


int PR_LoadProgs(progs_state_t *pr, char *filename, int version, int crc)
{
    int i;

    memset(pr, 0, sizeof(progs_state_t));

    CRC_Init(&pr->crc);

    pr->progs = (dprograms_t *)COM_LoadHunkFile(filename);
    if (!pr->progs)
    {
        Con_Printf("PR_LoadProgs: couldn't load progs.dat");
        return 1;
    }
    Con_DPrintf("Programs occupy %iK.\n", com_filesize / 1024);

    for (i = 0; i < com_filesize; i++)
    {
        CRC_ProcessByte(&pr->crc, ((byte *)pr->progs)[i]);
    }

    // byte swap the header
    for (i = 0; i < sizeof(*pr->progs) / sizeof(int32_t); i++)
    {
        ((int32_t *)pr->progs)[i] = LittleLong(((int32_t *)pr->progs)[i]);
    }

    if (version != 0 && version != pr->progs->version)
    {
        Con_Printf("progs.dat has wrong version number (%i should be %i)", pr->progs->version, version);
        return 1;
    }
    if (crc != 0 && crc != pr->progs->crc)
    {
        Con_Printf("progs.dat system vars have been modified, progdefs.h is out of date");
        return 1;
    }

    pr->functions = (dfunction_t *)((byte *)pr->progs + pr->progs->ofs_functions);
    pr->strings = (char *)pr->progs + pr->progs->ofs_strings;
    pr->globaldefs = (ddef_t *)((byte *)pr->progs + pr->progs->ofs_globaldefs);
    pr->fielddefs = (ddef_t *)((byte *)pr->progs + pr->progs->ofs_fielddefs);
    pr->statements = (dstatement_t *)((byte *)pr->progs + pr->progs->ofs_statements);
    pr->globals = (float *)((byte *)pr->progs + pr->progs->ofs_globals);

    *(size_t *)&pr->edict_size = sizeof(edict_t) + pr->progs->entityfields * sizeof(int32_t);

    // byte swap the lumps
    for (i = 0; i < pr->progs->numstatements; i++)
    {
        pr->statements[i].op = LittleShort(pr->statements[i].op);
        pr->statements[i].a = LittleShort(pr->statements[i].a);
        pr->statements[i].b = LittleShort(pr->statements[i].b);
        pr->statements[i].c = LittleShort(pr->statements[i].c);
    }

    for (i = 0; i < pr->progs->numfunctions; i++)
    {
        pr->functions[i].first_statement = LittleLong(pr->functions[i].first_statement);
        pr->functions[i].parm_start = LittleLong(pr->functions[i].parm_start);
        pr->functions[i].s_name = LittleLong(pr->functions[i].s_name);
        pr->functions[i].s_file = LittleLong(pr->functions[i].s_file);
        pr->functions[i].numparms = LittleLong(pr->functions[i].numparms);
        pr->functions[i].locals = LittleLong(pr->functions[i].locals);
    }

    for (i = 0; i < pr->progs->numglobaldefs; i++)
    {
        pr->globaldefs[i].type = LittleShort(pr->globaldefs[i].type);
        pr->globaldefs[i].ofs = LittleShort(pr->globaldefs[i].ofs);
        pr->globaldefs[i].s_name = LittleLong(pr->globaldefs[i].s_name);
    }

    for (i = 0; i < pr->progs->numfielddefs; i++)
    {
        pr->fielddefs[i].type = LittleShort(pr->fielddefs[i].type);
        if (pr->fielddefs[i].type & DEF_SAVEGLOBAL)
        {
            Con_Printf("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
            return 1;
        }
        pr->fielddefs[i].ofs = LittleShort(pr->fielddefs[i].ofs);
        pr->fielddefs[i].s_name = LittleLong(pr->fielddefs[i].s_name);
    }

    for (i = 0; i < pr->progs->numglobals; i++)
    {
        ((int32_t *)pr->globals)[i] = LittleLong(((int32_t *)pr->globals)[i]);
    }

    return 0;
}


void PR_BuildStructs(progs_state_t *pr, uint32_t *globalStruct, char **globalStrings, uint32_t *fieldStruct, char **fieldStrings)
{
    int i;
    ddef_t *def;

    if (globalStruct)
    {
        /* Get the offsets of the global vars. */
        for (i = 0; globalStrings[i] != NULL; i++)
        {
            def = PR_FindGlobal(pr, globalStrings[i]);
            if (!def)
            {
                globalStruct[i] = 0;
                Con_DPrintf("PR_LoadProgs: progs.dat globaldefs missing %s\n", globalStrings[i]);
                continue;
            }
            if (def->type != ev_function)
            {
                globalStruct[i] = def->ofs;
            }
            else
            {
                /* C code passes these to PR_ExecuteProgram directly. */
                globalStruct[i] = *(int32_t *)(pr->globals + def->ofs);
            }
        }
        pr->global_struct = globalStruct;
    }

    if (fieldStruct)
    {
        /* Get the offsets of the entity vars. */
        for (i = 0; fieldStrings[i] != NULL; i++)
        {
            def = PR_FindField(pr, fieldStrings[i]);
            if (!def)
            {
                fieldStruct[i] = 0;
                Con_DPrintf("PR_LoadProgs: progs.dat fielddefs missing %s\n", fieldStrings[i]);
                continue;
            }
            fieldStruct[i] = def->ofs;
        }
        pr->field_struct = fieldStruct;
    }
}


void PR_Init()
{
    Cmd_AddCommand("profile", PR_Profile_f);
}

