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

#include "globals.h"
#include "args.h"
#include "input.h"
#include "errors.h"
#include "perf.h"
#include "symtab.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <elf.h>
#include <assert.h>

void print_refs(struct input* in)
{
    assert( in );
    assert( in->map != NULL && in->map != MAP_FAILED );

    struct reader_funcs rdr = read_elf_header(in);

    rdr.find_sections(in);
    struct symtab* st = rdr.read_symtab(in);
    if (st) {
	rdr.process_relocations(in, st);
	dump_symtab(st);
	free_symtab(st);
    }

    // TODO: add timing infrastructure
    // TODO: add option to filter by symbol name (strstr())
}

int main(int argc, char* argv[])
{
    init_globals(argc, argv);

    struct input in;
    init_input(&in);

    if ( ! parse_args(argc, argv, &in) ) {
	usage();
	return EXIT_FAILURE;
    }

    open_input(&in);

    print_refs(&in);

    print_memstats(); // do it here before we have free'ed everything

    fini_input(&in);
    fini_globals();

    return EXIT_SUCCESS;
}

