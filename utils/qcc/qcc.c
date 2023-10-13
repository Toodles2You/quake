/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#include "qcc.h"


char		destfile[1024];

float		pr_globals[MAX_REGS];
int			numpr_globals;

char		strings[MAX_STRINGS];
int			strofs;

dstatement_t	statements[MAX_STATEMENTS];
int			numstatements;
int			statement_linenums[MAX_STATEMENTS];

dfunction_t	functions[MAX_FUNCTIONS];
int			numfunctions;

ddef_t		globals[MAX_GLOBALS];
int			numglobaldefs;

ddef_t		fields[MAX_FIELDS];
int			numfielddefs;

char		precache_sounds[MAX_SOUNDS][MAX_DATA_PATH];
int			precache_sounds_block[MAX_SOUNDS];
int			numsounds;

char		precache_models[MAX_MODELS][MAX_DATA_PATH];
int			precache_models_block[MAX_SOUNDS];
int			nummodels;

char		precache_files[MAX_FILES][MAX_DATA_PATH];
int			precache_files_block[MAX_SOUNDS];
int			numfiles;


/*
=================
BspModels

Runs qbsp and light on all of the models with a .bsp extension
=================
*/
void BspModels (void)
{
	int		p;
	char	*gamedir;
	int		i;
	char	*m;
	char	cmd[1024];
	char	name[256];

	p = CheckParm ("-bspmodels");
	if (!p)
		return;
	if (p == myargc-1)
		Error ("-bspmodels must preceed a game directory");
	gamedir = myargv[p+1];
	
	for (i=0 ; i<nummodels ; i++)
	{
		m = precache_models[i];
		if (strcmp(m+strlen(m)-4, ".bsp"))
			continue;
		strcpy (name, m);
		name[strlen(m)-4] = 0;
		sprintf (cmd, "qbsp %s/%s ; light -extra %s/%s", gamedir, name, gamedir, name);
		system (cmd);
	}
}

// CopyString returns an offset from the string heap
int	CopyString (char *str)
{
	int		old;
	
	old = strofs;
	strcpy (strings+strofs, str);
	strofs += strlen(str)+1;
	return old;
}

void PrintStrings (void)
{
	int		i, l, j;
	
	for (i=0 ; i<strofs ; i += l)
	{
		l = strlen(strings+i) + 1;
		printf ("%5i : ",i);
		for (j=0 ; j<l ; j++)
		{
			if (strings[i+j] == '\n')
			{
				putchar ('\\');
				putchar ('n');
			}
			else
				putchar (strings[i+j]);
		}
		printf ("\n");
	}
}


void PrintFunctions (void)
{
	int		i,j;
	dfunction_t	*d;
	
	for (i=0 ; i<numfunctions ; i++)
	{
		d = &functions[i];
		printf ("%s : %s : %i %i (", strings + d->s_file, strings + d->s_name, d->first_statement, d->parm_start);
		for (j=0 ; j<d->numparms ; j++)
			printf ("%i ",d->parm_size[j]);
		printf (")\n");
	}
}

void PrintFields (void)
{
	int		i;
	ddef_t	*d;
	
	for (i=0 ; i<numfielddefs ; i++)
	{
		d = &fields[i];
		printf ("%5i : (%i) %s\n", d->ofs, d->type, strings + d->s_name);
	}
}

void PrintGlobals (void)
{
	int		i;
	ddef_t	*d;
	
	for (i=0 ; i<numglobaldefs ; i++)
	{
		d = &globals[i];
		printf ("%5i : (%i) %s\n", d->ofs, d->type, strings + d->s_name);
	}
}


void InitData (void)
{
	int		i;
	
	numstatements = 1;
	strofs = 1;
	numfunctions = 1;
	numglobaldefs = 1;
	numfielddefs = 1;
	
	def_ret.ofs = OFS_RETURN;
	for (i=0 ; i<MAX_PARMS ; i++)
		def_parms[i].ofs = OFS_PARM0 + 3*i;
}


void WriteData (int crc)
{
	def_t		*def;
	ddef_t		*dd;
	dprograms_t	progs;
	int			h;
	int			i;

	for (def = pr.def_head.next ; def ; def = def->next)
	{
		if (def->type->type == ev_function)
		{
//			df = &functions[numfunctions];
//			numfunctions++;

		}
		else if (def->type->type == ev_field)
		{
			dd = &fields[numfielddefs];
			numfielddefs++;
			dd->type = def->type->aux_type->type;
			dd->s_name = CopyString (def->name);
			dd->ofs = G_INT(def->ofs);
		}
		dd = &globals[numglobaldefs];
		numglobaldefs++;
		dd->type = def->type->type;
		if ( !def->initialized
		&& def->type->type != ev_function
		&& def->type->type != ev_field
		&& def->scope == NULL)
			dd->type |= DEF_SAVEGLOBAL;
		dd->s_name = CopyString (def->name);
		dd->ofs = def->ofs;
	}

//PrintStrings ();
//PrintFunctions ();
//PrintFields ();
//PrintGlobals ();
strofs = (strofs+3)&~3;

	printf ("%6i strofs\n", strofs);
	printf ("%6i numstatements\n", numstatements);
	printf ("%6i numfunctions\n", numfunctions);
	printf ("%6i numglobaldefs\n", numglobaldefs);
	printf ("%6i numfielddefs\n", numfielddefs);
	printf ("%6i numpr_globals\n", numpr_globals);
	
	h = SafeOpenWrite (destfile);
	SafeWrite (h, &progs, sizeof(progs));

	progs.ofs_strings = lseek (h, 0, SEEK_CUR);
	progs.numstrings = strofs;
	SafeWrite (h, strings, strofs);

	progs.ofs_statements = lseek (h, 0, SEEK_CUR);
	progs.numstatements = numstatements;
	for (i=0 ; i<numstatements ; i++)
	{
		statements[i].op = LittleShort(statements[i].op);
		statements[i].a = LittleShort(statements[i].a);
		statements[i].b = LittleShort(statements[i].b);
		statements[i].c = LittleShort(statements[i].c);
	}
	SafeWrite (h, statements, numstatements*sizeof(dstatement_t));

	progs.ofs_functions = lseek (h, 0, SEEK_CUR);
	progs.numfunctions = numfunctions;
	for (i=0 ; i<numfunctions ; i++)
	{
	functions[i].first_statement = LittleLong (functions[i].first_statement);
	functions[i].parm_start = LittleLong (functions[i].parm_start);
	functions[i].s_name = LittleLong (functions[i].s_name);
	functions[i].s_file = LittleLong (functions[i].s_file);
	functions[i].numparms = LittleLong (functions[i].numparms);
	functions[i].locals = LittleLong (functions[i].locals);
	}	
	SafeWrite (h, functions, numfunctions*sizeof(dfunction_t));

	progs.ofs_globaldefs = lseek (h, 0, SEEK_CUR);
	progs.numglobaldefs = numglobaldefs;
	for (i=0 ; i<numglobaldefs ; i++)
	{
		globals[i].type = LittleShort (globals[i].type);
		globals[i].ofs = LittleShort (globals[i].ofs);
		globals[i].s_name = LittleLong (globals[i].s_name);
	}
	SafeWrite (h, globals, numglobaldefs*sizeof(ddef_t));

	progs.ofs_fielddefs = lseek (h, 0, SEEK_CUR);
	progs.numfielddefs = numfielddefs;
	for (i=0 ; i<numfielddefs ; i++)
	{
		fields[i].type = LittleShort (fields[i].type);
		fields[i].ofs = LittleShort (fields[i].ofs);
		fields[i].s_name = LittleLong (fields[i].s_name);
	}
	SafeWrite (h, fields, numfielddefs*sizeof(ddef_t));

	progs.ofs_globals = lseek (h, 0, SEEK_CUR);
	progs.numglobals = numpr_globals;
	for (i=0 ; i<numpr_globals ; i++)
		((int32_t *)pr_globals)[i] = LittleLong (((int32_t *)pr_globals)[i]);
	SafeWrite (h, pr_globals, numpr_globals*4);

	printf ("%6i TOTAL SIZE\n", (int)lseek (h, 0, SEEK_CUR));	

	progs.entityfields = pr.size_fields;

	progs.version = PROG_VERSION;
	progs.crc = crc;
	
// byte swap the header and write it out
	for (i=0 ; i<sizeof(progs)/4 ; i++)
		((int32_t *)&progs)[i] = LittleLong ( ((int32_t *)&progs)[i] );		
	lseek (h, 0, SEEK_SET);
	SafeWrite (h, &progs, sizeof(progs));
	close (h);
	
}



/*
===============
PR_String

Returns a string suitable for printing (no newlines, max 60 chars length)
===============
*/
char *PR_String (char *string)
{
	static char buf[80];
	char	*s;
	
	s = buf;
	*s++ = '"';
	while (string && *string)
	{
		if (s == buf + sizeof(buf) - 2)
			break;
		if (*string == '\n')
		{
			*s++ = '\\';
			*s++ = 'n';
		}
		else if (*string == '"')
		{
			*s++ = '\\';
			*s++ = '"';
		}
		else
			*s++ = *string;
		string++;
		if (s - buf > 60)
		{
			*s++ = '.';
			*s++ = '.';
			*s++ = '.';
			break;
		}
	}
	*s++ = '"';
	*s++ = 0;
	return buf;
}



def_t	*PR_DefForFieldOfs (gofs_t ofs)
{
	def_t	*d;
	
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (d->type->type != ev_field)
			continue;
		if (*((int32_t *)&pr_globals[d->ofs]) == ofs)
			return d;
	}
	Error ("PR_DefForFieldOfs: couldn't find %i",ofs);
	return NULL;
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *PR_ValueString (etype_t type, void *val)
{
	static char	line[256];
	def_t		*def;
	dfunction_t	*f;
	
	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_String(strings + *(int32_t *)val));
		break;
	case ev_entity:	
		sprintf (line, "entity %i", *(int32_t *)val);
		break;
	case ev_function:
		f = functions + *(int32_t *)val;
		if (!f)
			sprintf (line, "undefined function");
		else
			sprintf (line, "%s()", strings + f->s_name);
		break;
	case ev_field:
		def = PR_DefForFieldOfs ( *(int32_t *)val );
		sprintf (line, ".%s", def->name);
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%5.1f", *(float *)val);
		break;
	case ev_vector:
		sprintf (line, "'%5.1f %5.1f %5.1f'", ((float *)val)[0], ((float *)val)[1], ((float *)val)[2]);
		break;
	case ev_pointer:
		sprintf (line, "pointer");
		break;
	default:
		sprintf (line, "bad type %i", type);
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
char *PR_GlobalStringNoContents (gofs_t ofs)
{
	int		i;
	def_t	*def;
	void	*val;
	static char	line[128];
	
	val = (void *)&pr_globals[ofs];
	def = pr_global_defs[ofs];
	if (!def)
//		Error ("PR_GlobalString: no def for %i", ofs);
		sprintf (line,"%i(???)", ofs);
	else
		sprintf (line,"%i(%s)", ofs, def->name);
	
	i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");
		
	return line;
}

char *PR_GlobalString (gofs_t ofs)
{
	char	*s;
	int		i;
	def_t	*def;
	void	*val;
	static char	line[128];
	
	val = (void *)&pr_globals[ofs];
	def = pr_global_defs[ofs];
	if (!def)
		return PR_GlobalStringNoContents(ofs);
	if (def->initialized && def->type->type != ev_function)
	{
		s = PR_ValueString (def->type->type, &pr_globals[ofs]);
		sprintf (line,"%i(%s)", ofs, s);
	}
	else
		sprintf (line,"%i(%s)", ofs, def->name);
	
	i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");
		
	return line;
}

/*
============
PR_PrintOfs
============
*/
void PR_PrintOfs (gofs_t ofs)
{
	printf ("%s\n",PR_GlobalString(ofs));
}

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement (dstatement_t *s)
{
	int		i;
	
	printf ("%4i : %4i : %s ", (int)(s - statements), statement_linenums[s-statements], pr_opcodes[s->op].opname);
	i = strlen(pr_opcodes[s->op].opname);
	for ( ; i<10 ; i++)
		printf (" ");
		
	if (s->op == OP_IF || s->op == OP_IFNOT)
		printf ("%sbranch %i",PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		printf ("branch %i",s->a);
	}
	else if ( (unsigned)(s->op - OP_STORE_F) < 6)
	{
		printf ("%s",PR_GlobalString(s->a));
		printf ("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			printf ("%s",PR_GlobalString(s->a));
		if (s->b)
			printf ("%s",PR_GlobalString(s->b));
		if (s->c)
			printf ("%s", PR_GlobalStringNoContents(s->c));
	}
	printf ("\n");
}


/*
============
PR_PrintDefs
============
*/
void PR_PrintDefs (void)
{
	def_t	*d;
	
	for (d=pr.def_head.next ; d ; d=d->next)
		PR_PrintOfs (d->ofs);
}


/*
==============
PR_BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void	PR_BeginCompilation (void *memory, int memsize)
{
	int		i;
	
	pr.memory = memory;
	pr.max_memory = memsize;
	
	numpr_globals = RESERVED_OFS;
	pr.def_tail = &pr.def_head;
	
	for (i=0 ; i<RESERVED_OFS ; i++)
		pr_global_defs[i] = &def_void;
		
// link the function type in so state forward declarations match proper type
	pr.types = &type_function;
	type_function.next = NULL;
	pr_error_count = 0;
}

/*
==============
PR_FinishCompilation

called after all files are compiled to check for errors
Returns false if errors were detected.
==============
*/
bool	PR_FinishCompilation (void)
{
	def_t		*d;
	bool	errors;
	
	errors = false;
	
// check to make sure all functions prototyped have code
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (d->type->type == ev_function && !d->scope)// function parms are ok
		{
//			f = G_FUNCTION(d->ofs);
//			if (!f || (!f->code && !f->builtin) )
			if (!d->initialized)
			{
				printf ("function %s was not defined\n",d->name);
				errors = true;
			}
		}
	}

	return !errors;
}

/*
============
PR_WriteProgdefs

Writes the global and entity structures out
Returns a crc of the header, to be stored in the progs file for comparison
at load time.
============
*/
int	PR_WriteProgdefs (char *filename)
{
	def_t	*d;
	FILE	*f;
	unsigned short		crc;
	int		c;
	
	printf ("writing %s\n", filename);
	f = fopen (filename, "w");
	
// print global vars until the first field is defined
	fprintf (f,"\n/* file generated by qcc, do not modify */\n\ntypedef struct\n{\tint\tpad[%i];\n", RESERVED_OFS);
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_globals"))
			break;
			
		switch (d->type->type)
		{
		case ev_float:
			fprintf (f, "\tfloat\t%s;\n",d->name);
			break;
		case ev_vector:
			fprintf (f, "\tvec3_t\t%s;\n",d->name);
			d=d->next->next->next;	// skip the elements
			break;
		case ev_string:
			fprintf (f,"\tstring_t\t%s;\n",d->name);
			break;
		case ev_function:
			fprintf (f,"\tfunc_t\t%s;\n",d->name);
			break;
		case ev_entity:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		default:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		}
	}
	fprintf (f,"} globalvars_t;\n\n");

// print all fields
	fprintf (f,"typedef struct\n{\n");
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_fields"))
			break;
			
		if (d->type->type != ev_field)
			continue;
			
		switch (d->type->aux_type->type)
		{
		case ev_float:
			fprintf (f,"\tfloat\t%s;\n",d->name);
			break;
		case ev_vector:
			fprintf (f,"\tvec3_t\t%s;\n",d->name);
			d=d->next->next->next;	// skip the elements
			break;
		case ev_string:
			fprintf (f,"\tstring_t\t%s;\n",d->name);
			break;
		case ev_function:
			fprintf (f,"\tfunc_t\t%s;\n",d->name);
			break;
		case ev_entity:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		default:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		}
	}
	fprintf (f,"} entvars_t;\n\n");
	
	fclose (f);
	
// do a crc of the file
	CRC_Init (&crc);
	f = fopen (filename, "r+");
	while ((c = fgetc(f)) != EOF)
		CRC_ProcessByte (&crc, c);
		
	fprintf (f,"#define PROGHEADER_CRC %i\n", crc);
	fclose (f);

	return crc;
}


void PrintFunction (char *name)
{
	int		i;
	dstatement_t	*ds;
	dfunction_t		*df;
	
	for (i=0 ; i<numfunctions ; i++)
		if (!strcmp (name, strings + functions[i].s_name))
			break;
	if (i==numfunctions)
		Error ("No function names \"%s\"", name);
	df = functions + i;	
	
	printf ("Statements for %s:\n", name);
	ds = statements + df->first_statement;
	while (1)
	{
		PR_PrintStatement (ds);
		if (!ds->op)
			break;
		ds++;
	}
}

/*
==============================================================================

DIRECTORY COPYING / PACKFILE CREATION

==============================================================================
*/

typedef struct
{
	char	name[56];
	int		filepos, filelen;
} packfile_t;

typedef struct
{
	char	id[4];
	int		dirofs;
	int		dirlen;
} packheader_t;

packfile_t	pfiles[4096], *pf;
int			packhandle;
int			packbytes;


/*
===========
PackFile

Copy a file into the pak file
===========
*/
void PackFile (char *src, char *name)
{
	int		in;
	int		remaining, count;
	char	buf[4096];
	
	if ( (byte *)pf - (byte *)pfiles > sizeof(pfiles) )
		Error ("Too many files in pak file");
	
	in = SafeOpenRead (src);
	remaining = filelength (in);

	pf->filepos = LittleLong (lseek (packhandle, 0, SEEK_CUR));
	pf->filelen = LittleLong (remaining);
	strcpy (pf->name, name);
	printf ("%64s : %7i\n", pf->name, remaining);

	packbytes += remaining;
	
	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		SafeRead (in, buf, count);
		SafeWrite (packhandle, buf, count);
		remaining -= count;
	}

	close (in);
	pf++;
}


/*
===========
CopyFiles
===========
*/
void CopyFiles (void)
{
	int		i, p;
	char	srcdir[1024], destdir[1024];
	char	srcfile[1024], destfile[1024];
	int		copytype;
	char	name[1024];
	packheader_t	header;
	int		dirlen;
	int		blocknum;
	unsigned short		crc;

	printf ("%3i unique precache_sounds\n", numsounds);
	printf ("%3i unique precache_models\n", nummodels);
	
	copytype = 0;

	p = CheckParm ("-copy");
	if (p && p < myargc-2)
	{	// create a new directory tree
		copytype = 1;

		strcpy (srcdir, myargv[p+1]);
		strcpy (destdir, myargv[p+2]);
		if (srcdir[strlen(srcdir)-1] != '/')
			strcat (srcdir, "/");
		if (destdir[strlen(destdir)-1] != '/')
			strcat (destdir, "/");
	}

	blocknum = 1;
	p = CheckParm ("-pak2");
	if (p && p <myargc-2)
		blocknum = 2;
	else
		p = CheckParm ("-pak");
	if (p && p < myargc-2)
	{	// create a pak file
		strcpy (srcdir, myargv[p+1]);
		strcpy (destdir, myargv[p+2]);
		if (srcdir[strlen(srcdir)-1] != '/')
			strcat (srcdir, "/");
		DefaultExtension (destdir, ".pak");

		pf = pfiles;
		packhandle = SafeOpenWrite (destdir);
		SafeWrite (packhandle, &header, sizeof(header));	
		copytype = 2;
	}
	
	if (!copytype)
		return;
				
	for (i=0 ; i<numsounds ; i++)
	{
		if (precache_sounds_block[i] != blocknum)
			continue;
		sprintf (name, "sound/%s", precache_sounds[i]);
		sprintf (srcfile,"%s%s",srcdir, name);
		sprintf (destfile,"%s%s",destdir, name);
		if (copytype == 1)
			CopyFile (srcfile, destfile);
		else
			PackFile (srcfile, name);
	}
	for (i=0 ; i<nummodels ; i++)
	{
		if (precache_models_block[i] != blocknum)
			continue;
		sprintf (srcfile,"%s%s",srcdir, precache_models[i]);
		sprintf (destfile,"%s%s",destdir, precache_models[i]);
		if (copytype == 1)
			CopyFile (srcfile, destfile);
		else
			PackFile (srcfile, precache_models[i]);
	}
	for (i=0 ; i<numfiles ; i++)
	{
		if (precache_files_block[i] != blocknum)
			continue;
		sprintf (srcfile,"%s%s",srcdir, precache_files[i]);
		sprintf (destfile,"%s%s",destdir, precache_files[i]);
		if (copytype == 1)
			CopyFile (srcfile, destfile);
		else
			PackFile (srcfile, precache_files[i]);
	}
	
	if (copytype == 2)
	{
		header.id[0] = 'P';
		header.id[1] = 'A';
		header.id[2] = 'C';
		header.id[3] = 'K';
		dirlen = (byte *)pf - (byte *)pfiles;
		header.dirofs = LittleLong(lseek (packhandle, 0, SEEK_CUR));
		header.dirlen = LittleLong(dirlen);
		
		SafeWrite (packhandle, pfiles, dirlen);
	
		lseek (packhandle, 0, SEEK_SET);
		SafeWrite (packhandle, &header, sizeof(header));
		close (packhandle);	
	
	// do a crc of the file
		CRC_Init (&crc);
		for (i=0 ; i<dirlen ; i++)
			CRC_ProcessByte (&crc, ((byte *)pfiles)[i]);
	
		i = pf - pfiles;
		printf ("%i files packed in %i bytes (%i crc)\n",i, packbytes, crc);
	}
}

//============================================================================

/*
============
main
============
*/
void main (int argc, char **argv)
{
	char	*src;
	char	*src2;
	char	filename[1024];
	int		p, crc;
	char	sourcedir[1024];

	myargc = argc;
	myargv = argv;
	
	if ( CheckParm ("-?") || CheckParm ("-help"))
	{
		printf ("qcc looks for progs.src in the current directory.\n");
		printf ("to look in a different directory: qcc -src <directory>\n");
		printf ("to build a clean data tree: qcc -copy <srcdir> <destdir>\n");
		printf ("to build a clean pak file: qcc -pak <srcdir> <packfile>\n");
		printf ("to bsp all bmodels: qcc -bspmodels <gamedir>\n");
		return;
	}
	
	p = CheckParm ("-src");
	if (p && p < argc-1 )
	{
		strcpy (sourcedir, argv[p+1]);
		strcat (sourcedir, "/");
		printf ("Source directory: %s\n", sourcedir);
	}
	else
		strcpy (sourcedir, "");

	InitData ();
	
	sprintf (filename, "%sprogs.src", sourcedir);
	LoadFile (filename, (void *)&src);
	
	src = COM_Parse (src);
	if (!src)
		Error ("No destination filename.  qcc -help for info.\n");
	strcpy (destfile, com_token);
	printf ("outputfile: %s\n", destfile);
	
	pr_dumpasm = false;

	PR_BeginCompilation (malloc (0x100000), 0x100000);

// compile all the files
	do
	{
		src = COM_Parse(src);
		if (!src)
			break;
		sprintf (filename, "%s%s", sourcedir, com_token);
		printf ("compiling %s\n", filename);
		LoadFile (filename, (void *)&src2);

		if (!PR_CompileFile (src2, filename) )
			exit (1);
			
	} while (1);
	
	if (!PR_FinishCompilation ())
		Error ("compilation errors");

	p = CheckParm ("-asm");
	if (p)
	{
		for (p++ ; p<argc ; p++)
		{
			if (argv[p][0] == '-')
				break;
			PrintFunction (argv[p]);
		}
	}
	
	
// write progdefs.h
	crc = PR_WriteProgdefs ("progdefs.h");
	
// write data file
	WriteData (crc);
	
// regenerate bmodels if -bspmodels
	BspModels ();

// report / copy the data files
	CopyFiles ();
}
