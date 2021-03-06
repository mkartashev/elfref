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
#include "errors.h"

#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <limits.h>

static char *	prg_name;	// the name of self for error reporting

/**
 * Releses the global resources allocated by glob_init(). Should be called right before exit.
 */
static void		glob_fini(void)
{
	// Note: can't call fatal() from here or we will recurse

	if (prg_name)
	{
		free(prg_name);
	}
}

/**
 * Initializes global state of the program. Must be called at the very start of main().
 */
extern void 		glob_init(int argc __attribute__((unused)), char *argv[])
{
	char *self_full = strndupa(argv[0], PATH_MAX);
	char *self = basename(self_full);
	prg_name = strdup(self); // remember our basename for messages

	atexit(glob_fini);
}

/**
 * Returns the name of this program for the purpose of prefixing messages.
 */
extern const char *	glob_get_program_name(void)
{
	return prg_name;
}
