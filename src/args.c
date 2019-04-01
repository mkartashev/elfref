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

#include "args.h"
#include "errors.h"
#include "globals.h"
#include "symtab.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

static const char *usage_str = 
"Usage: %s [OPTIONS]... ELF-FILE\n"
"\tfind what symbols (funcs and global variables) reference in ELF-FILE\n"
"\n"
"Options:\n"
"    -s pattern\tonly show info about symbols of which pattern is a substring\n"
"    -f\t\tonly show info about functions (symbol type FUNC);\n"
"    \t\tby default, OBJECTs are also shown\n"
"    -d\t\tprint offsets in decimal instead of hex\n"
"    -h\t\tdisplay help\n"
"    -v\t\tverbose output\n"
"    -vv\t\tverbose and debug output\n"
;

void usage()
{
    printf(usage_str, globals.prg_name);
    printf("\nOutput format:\n");
    print_legend();
}

bool parse_args(int argc, char* argv[], struct input* in)
{
    assert( in );

    for(int i = 1; i < argc; ++i) {
	const char* arg = argv[i];

	if ( strcmp(arg, "-v") == 0 ) {
	    globals.opts.verbosity = VERB;
	} else if ( strcmp(arg, "-vv") == 0 ) {
	    globals.opts.verbosity = DBG;
	} else if ( strcmp(arg, "-f") == 0 ) {
	    globals.opts.funcs_only = true;
	} else if ( strcmp(arg, "-d") == 0 ) {
	    globals.opts.offsets_decimal = true;
	} else if ( strcmp(arg, "-h") == 0 ) {
	    return false;
	} else if ( strcmp(arg, "-s") == 0 ) {
	    i++;
	    if ( i < argc ) {
		globals.opts.name_pattern = argv[i];
	    } else {
		report(NORM, "-s option requires argument");
		return false;
	    }
	} else {
	    if ( ! globals.opts.fname ) {
		globals.opts.fname = arg;
	    } else {
		report(NORM, "Only one file name argument is supported (%s will be ignored)", arg);
	    }
	}
    }

    if ( !globals.opts.fname ) {
	report(NORM, "ELF file name required");
	return false;
    }

    return true;
}

