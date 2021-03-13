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

#include <assert.h>
#include <stdlib.h>

static void	print_refs(input_t* in)
{
	assert(in);

	struct reader_funcs rdr = input_read_elf_header(in);

	elf_sections_t * sec = rdr.find_sections(in);
	symtab_t *st = rdr.read_symtab(in, sec);
	if (st)
	{
		rdr.process_relocations(in, sec, st);
		symtab_dump(st);
		symtab_free(st);
	}
}

extern int	main(int argc, char* argv[])
{
	int rc = EXIT_SUCCESS;

	glob_init(argc, argv);
	args_init();

	input_t * in = input_init();

	if (args_parse(argc, argv, in))
	{
		input_open(in);

		print_refs(in);

		perf_print_memstats(); // do it here before we have free'ed everything

		input_close(in);
	}
	else
	{
		args_usage();
		rc = EXIT_FAILURE;
	}

	return rc;
}
