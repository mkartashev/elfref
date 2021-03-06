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

#include "errors.h"
#include "globals.h"
#include "args.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

/**
 * Issues the given error message to stderr using printf() formatting.
 */
extern void	error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: Error: ", glob_get_program_name());
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

/**
 * Issues the given informational message to stderr using printf() formatting.
 * The message is only issued if the verbosity level given is at least as high as requested by the user
 * (see args_get_verbosity()).
 */
extern void	report(enum Verbosity v, const char *fmt, ...)
{
	if (args_get_verbosity() >= v)
	{
		va_list ap;

		va_start(ap, fmt);
		fprintf(stderr, "%s: ", glob_get_program_name());
		vfprintf(stderr, fmt, ap);
		fputc('\n', stderr);
		va_end(ap);
	}
}

/**
 * Issues a fatal error message to stderr using printf() formatting, performs cleanup and exists
 * with the failure exit code.
 */
extern void	fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: fatal error: ", glob_get_program_name());
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);

	exit(EXIT_FAILURE);
}

/**
 * Issues a fatal error message to stderr coupled with errno description, performs cleanup and exists
 * with the failure exit code.
 */
extern void	fatal_err(const char *msg)
{
	const bool was_error = (errno > 0);
	const char *errdescr = strerror(errno);

	// Not using fprintf() here because this function may be called what malloc() failed and fprintf() might try to allocate memory.
	fputs(glob_get_program_name(), stderr);
	fputs(": fatal error : ", stderr);
	fputs(msg, stderr);
	fputs(" (", stderr);
	fputs(errdescr, stderr);
	fputs(")\n", stderr);

	assert(was_error);

	exit(EXIT_FAILURE);
}
