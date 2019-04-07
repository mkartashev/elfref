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
#include "globals.h"

#include <stdlib.h>
#include <assert.h>

static int sym_compare(const void* s1, const void* s2)
{
    const sym* sym1 = (const sym*)s1;
    const sym* sym2 = (const sym*)s2;

    return sym1->offset > sym2->offset 
           ? 1 
	   : ( sym1->offset == sym2->offset 
	       ? 0
	       : -1 );

}

void sort_symtab(struct symtab* st)
{
    assert(st);
    assert(st->nsyms > 0);

    qsort(st->syms, st->nsyms, sizeof(sym), sym_compare);
}

struct symtab* alloc_symtab(size_t nsyms)
{
    sym* syms = calloc(nsyms, sizeof(sym));
    if ( ! syms ) {
	fatal_err("Not enough memory");
    }

    struct symtab* st = malloc(sizeof(struct symtab));
    if ( ! st ) {
	fatal_err("Not enough memory");
    }

    st->syms = syms;
    st->nsyms = nsyms;

    return st;
}

void free_symtab(struct symtab* s)
{
    assert(s);

    for(size_t i = 0; i < s->nsyms; ++i) {
	reloc* relocs = s->syms[i].relocs;
	while(relocs) {
	    reloc* next_reloc = relocs->next;
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
sym* locate_sym(struct symtab* st, size_t offset)
{
    sym* ret = NULL;

    size_t upper = st->nsyms;
    size_t lower = 0;
    size_t prev_i = upper;
    size_t i = 0;

    while (i != prev_i) {
	assert( upper > lower );
	prev_i = i;
	i = (upper-lower) / 2 + lower;

	ret = &st->syms[i];
	if ( offset < ret->offset ) {
	    upper = i;
	} else if ( offset > ret->offset ) {
	    lower = i;
	} else {
	    break; // exact match
	}
    }

    return ret;
}

void add_reloc(struct symtab* st, size_t offset, const char* sym_name,
               bool is_func, int64_t addend)
{
    assert(st);

    sym* s = locate_sym(st, offset);
    if ( s ) {
	// We're supposed to have found a relocation pointing into the symbol
	// 's', not before it
	assert( offset >= s->offset );

	reloc* new_r = malloc(sizeof(reloc));
	if ( ! new_r ) {
	    fatal("Out of memory");
	}
	new_r->sym_name = sym_name;
	new_r->is_func  = is_func;
	new_r->offset = offset - s->offset;
	new_r->addend = addend;
	new_r->next = NULL;

	// Find where to place this new relocation
	if ( !s->relocs ) {
	    s->relocs = new_r;
	} else {
	    struct reloc* r = s->relocs;
	    struct reloc* last_r = NULL;
	    for( ; r && r->offset < new_r->offset; r = r->next) {
		last_r = r;
	    }

	    if ( last_r ) {
		last_r->next = new_r;
	    } else {
		s->relocs = new_r; // add to the head
	    }
	    new_r->next = r;
	}
    } else {
	error("unable to locate sym corresponding to offset 0x%0lx", offset);
    }
}

void print_legend()
{
    fprintf(stdout, "sym-name (addr sym-value)    <-- symbol name and address\n");
    fprintf(stdout, " (+offset-from-sym) -> [sym-name[()]] [+addend]\n");
    fprintf(stdout, " ^                     ^               ^       \n");
    fprintf(stdout, " +- offset from sym    |               +- addend (for RELA relocations)\n");
    fprintf(stdout, "    start              +- name of referenced symbol; () means it's a function\n");
}

void dump_reloc(reloc* r)
{
    if ( globals.opts.offsets_decimal ) {
	fprintf(stdout, "\t(+%ld)-> ", r->offset);
    } else {
	fprintf(stdout, "\t(+0x%04lx)-> ", r->offset);
    }

    if ( r->sym_name ) {
	fprintf(stdout, "%s", r->sym_name);

	if ( r->is_func )
	    fprintf(stdout, "()");
    }

    // Not interested in seeing zero addend; but if it's 
    // all there is, print it anyway
    const bool show_addend = (r->addend != 0) || !r->sym_name;
    if ( show_addend )  {
	fprintf(stdout, "%ld", r->addend);
    }

    fprintf(stdout, "\n");
}

void dump_sym(sym* s)
{
    fprintf(stdout, "%s (addr 0x%08lx)\n", s->name, s->offset);

    for(reloc* r = s->relocs; r; r = r->next) {
	dump_reloc(r);
    }
}

void dump_symtab(struct symtab* st)
{
    assert(st);

    for(size_t i = 0; i < st->nsyms; ++i) {
	sym* s = &st->syms[i];
	if ( s->relocs ) {
	    dump_sym(s);
	}
    }
}

