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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

void error(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "%s: Error: ", globals.prg_name);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void report(enum Verbosity v, const char* fmt, ...)
{
    if ( globals.opts.verbosity >= v ) {
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", globals.prg_name);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
    }
}

void fatal(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "%s: fatal error: ", globals.prg_name);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);

    fini_globals();
    exit(EXIT_FAILURE);
}

void fatal_err(const char* msg)
{
    bool was_error = (errno > 0);
    const char* errdescr = strerror(errno);
    fprintf(stderr, "%s: fatal error: %s (%s)\n", globals.prg_name, msg, errdescr);
    assert( was_error );

    fini_globals();
    exit(EXIT_FAILURE);
}

