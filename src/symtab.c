/*
  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org/>
*/

#include "symtab.h"
#include "errors.h"
#include "args.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/**
 * Describes the properties of a relocation that are of interest. Keeps a linkd list of such records.
 */
typedef struct reloc
{
	const char *	sym_name;	// refers this symbol name
	bool		is_func;	// symbol is function?
	size_t		offset;		// from the patched sym
	int64_t		addend;		// relocation's addend, if rela
	struct reloc *	next;		// linked list of relocations
} reloc;

/**
 * Describes the properties of an ELF symbol that are of interest.
 */
typedef struct sym
{
	size_t		offset;	// address of the sym
	int 		type;	// type of the sym
	const char *	name;	// symbol's name
	struct reloc *	relocs;	// relocs that reference this symbol
} sym;

/**
 * Describes a collection of ELF symbols (symtab).
 */
typedef struct symtab_s
{
	sym *	syms;		// array of nsyms (or more) elements
	size_t	nsyms;		// number of actual symbols in the syms array
	size_t 	free_idx;	// index of the next "free" slot in the syms array
} symtab_s;

static int	sym_compare(const void* s1, const void* s2)
{
	const sym *sym1 = (const sym *)s1;
	const sym *sym2 = (const sym *)s2;

	return  (sym1->offset > sym2->offset)
		? 1
		: (sym1->offset == sym2->offset ? 0 : -1);
}

/**
 * Sorts the given symbol table based on symbols offset.
 */
extern void	symtab_sort(symtab_t* st)
{
	assert(st);
	assert(st->free_idx > 0);

	qsort(st->syms, st->free_idx, sizeof(sym), sym_compare);
}

/**
 * Allocates new symbol table capable of holding up to nsyms entries.
 * The allocated resources must be released with symtab_free().
 */
extern symtab_t*	symtab_alloc(size_t nsyms)
{
	sym *syms = calloc(nsyms, sizeof(sym));
	if (!syms)
	{
		fatal_err("Not enough memory");
	}

	symtab_s *st = malloc(sizeof(symtab_s));
	if (!st)
	{
		fatal_err("Not enough memory");
	}

	st->syms = syms;
	st->nsyms = nsyms;
	st->free_idx = 0;

	return st;
}

/**
 * Releases the resources allocated for the symbol table (see symtab_alloc()).
 */
extern void		symtab_free(symtab_t* s)
{
	assert(s);

	for (size_t i = 0; i < s->free_idx; ++i)
	{
		reloc *relocs = s->syms[i].relocs;
		while (relocs)
		{
			reloc *next_reloc = relocs->next;
			free(relocs);
			relocs = next_reloc;
		}
		s->syms[i].relocs = NULL;
	}

	free(s->syms);
	free(s);
}

/**
 * Returns symbol, which offset is not less than the given offset and is nearest
 * to it. If not found, returns NULL.
 */
static sym*	locate_sym(symtab_t* st, size_t offset)
{
	sym *ret = NULL;

	size_t upper = st->nsyms;
	size_t lower = 0;
	size_t prev_i = upper;
	size_t i = 0;

	while (i != prev_i)
	{
		assert(upper > lower);
		prev_i = i;
		i = (upper - lower) / 2 + lower;

		ret = &st->syms[i];
		if (offset < ret->offset)
		{
			upper = i;
		}
		else if (offset > ret->offset)
		{
			lower = i;
		}
		else
		{
			break; // exact match
		}
	}

	return ret;
}

/**
 * Adds a symbol with the given properties to the given symbol table.
 */
extern size_t		symtab_add_sym(symtab_t* symtab, size_t offset, int type, const char* sym_name)
{
	assert(symtab);
	assert(symtab->syms);
	assert(symtab->free_idx < symtab->nsyms);

	symtab->syms[symtab->free_idx].offset = offset;
	symtab->syms[symtab->free_idx].type = type;
	symtab->syms[symtab->free_idx].name = sym_name;
	symtab->syms[symtab->free_idx].relocs = NULL;

	return ++symtab->free_idx;
}

/**
 * Adds relocation information to the appropriate symbol (determined by the offset) in the given symbol table.
 */
extern void		symtab_add_reloc(symtab_t* st, size_t offset, const char* sym_name, bool is_func, int64_t addend)
{
	assert(st);

	sym *s = locate_sym(st, offset);
	if (s)
	{
		// We're supposed to have found a relocation pointing into the symbol
		// 's', not before it
		assert(offset >= s->offset);

		reloc *new_r = malloc(sizeof(reloc));
		if (!new_r)
		{
			fatal("Out of memory");
		}
		new_r->sym_name = sym_name;
		new_r->is_func = is_func;
		new_r->offset = offset - s->offset;
		new_r->addend = addend;
		new_r->next = NULL;

		// Find where to place this new relocation
		if (!s->relocs)
		{
			s->relocs = new_r;
		}
		else
		{
			struct reloc *r = s->relocs;
			struct reloc *last_r = NULL;
			for (; r && r->offset < new_r->offset; r = r->next)
			{
				last_r = r;
			}

			if (last_r)
			{
				last_r->next = new_r;
			}
			else
			{
				s->relocs = new_r; // add to the head
			}
			new_r->next = r;
		}
	}
	else
	{
		error("unable to locate sym corresponding to offset 0x%0lx", offset);
	}
}

/**
 * Prints out the legend in symbol table output format.
 */
extern void	symtab_print_legend()
{
	fprintf(stdout, "sym-name (addr sym-value)    <-- symbol name and address\n");
	fprintf(stdout, " (+offset-from-sym) -> [sym-name[()]] [+addend]\n");
	fprintf(stdout, " ^                     ^               ^       \n");
	fprintf(stdout, " +- offset from sym    |               +- addend (for RELA relocations)\n");
	fprintf(stdout, "    start              +- name of referenced symbol; () means it's a function\n");
}

static void	dump_reloc(reloc *r)
{
	if (args_get_is_offsets_decimal())
	{
		fprintf(stdout, "\t(+%ld)-> ", r->offset);
	}
	else
	{
		fprintf(stdout, "\t(+0x%04lx)-> ", r->offset);
	}

	if (r->sym_name)
	{
		fprintf(stdout, "%s", r->sym_name);

		if (r->is_func)
			fprintf(stdout, "()");
	}

	// Not interested in seeing zero addend; but if it's
	// all there is, print it anyway
	const bool show_addend = (r->addend != 0) || !r->sym_name;
	if (show_addend)
	{
		fprintf(stdout, "%+ld", r->addend);
	}

	fprintf(stdout, "\n");
}

static void	dump_sym(sym* s)
{
	fprintf(stdout, "%s (addr 0x%08lx)\n", s->name, s->offset);

	for (reloc *r = s->relocs; r; r = r->next)
	{
		dump_reloc(r);
	}
}

/**
 * Prints out the contents of the symbol table, excluding symbols that are not of interest to the user based on the options given.
 * See args_sym_is_interesting().
 */
extern void	symtab_dump(symtab_t* st)
{
	assert(st);

	bool empty_output = true;
	for (size_t i = 0; i < st->free_idx; ++i)
	{
		sym *s = &st->syms[i];

		if (args_sym_is_interesting(s->name, s->type))
		{
			if (s->relocs)
			{
				dump_sym(s);
				empty_output = false;
			}
		}
	}

	if (empty_output)
	{
		if (args_get_is_funcs_only() || args_get_name_pattern())
		{
			report(NORM, "No symbols that match pattern found in .symtab and .dynsym; nothing to do.");
		}
	}
}
