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

#ifndef SYMTAB_H_
#define SYMTAB_H_

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct reloc
{
    const char*	    sym_name;   // refers this symbol name
    bool	    is_func;	// symbol is function?
    size_t	    offset;	// from the patched sym
    int64_t	    addend;	// relocation's addend, if rela
    struct reloc*   next;	// linked list of relocations
} reloc;

typedef struct sym 
{
    size_t	    offset;	// address of the sym
    const char*	    name;	// symbol's name
    struct reloc*   relocs;	// relocs that reference this symbol
} sym;

struct symtab
{
    sym*   syms;    // array of nsyms (or more) elements
    size_t nsyms;   // number of actual symbols in the syms array
};

struct symtab* alloc_symtab(size_t nsyms);
void free_symtab(struct symtab* s);

void sort_symtab(struct symtab* s);
void dump_symtab(struct symtab* s);
void print_legend();

void add_reloc(struct symtab* symtab, size_t offset, const char* sym_name, 
	bool is_func, int64_t addend);

#endif

