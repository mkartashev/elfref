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
#include <stdio.h>
#include <elf.h>

static const char *	fname;			// name of input ELF file
static unsigned int	verbosity;
static const char *	name_pattern;		// if set, only report symbols matching this pattern
static bool		funcs_only;		// only report about functions
static bool		offsets_decimal;	// show offsets in the decimal form

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

/**
 * Prints out program's usage info.
 */
extern void 	args_usage(void)
{
	printf(usage_str, glob_get_program_name());
	printf("\nOutput format:\n");
	symtab_print_legend();
}

/**
 * Initializes the program arguments to their default state.
 * Must be called before any arguments are queried or parsed.
 */
extern void 	args_init(void)
{
	verbosity = NORM;
}

/**
 * Processes the program options, issues appropriate diagnostics and returns true if arguments are OK.
 */
extern bool	args_parse(int argc, char* argv[], input_t* in)
{
	assert(in);

	for (int i = 1; i < argc; ++i)
	{
		const char *arg = argv[i];

		if (strcmp(arg, "-v") == 0)
		{
			verbosity = VERB;
		}
		else if (strcmp(arg, "-vv") == 0)
		{
			verbosity = DBG;
		}
		else if (strcmp(arg, "-f") == 0)
		{
			funcs_only = true;
		}
		else if (strcmp(arg, "-d") == 0)
		{
			offsets_decimal = true;
		}
		else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-?") == 0 || strcmp(arg, "-h") == 0)
		{
			return false;
		}
		else if (strcmp(arg, "-s") == 0)
		{
			i++;
			if (i < argc)
			{
				name_pattern = argv[i];
			}
			else
			{
				report(NORM, "-s option requires argument");
				return false;
			}
		}
		else
		{
			if (!fname)
			{
				fname = arg;
			}
			else
			{
				report(NORM, "Only one file name argument is supported (%s will be ignored)", arg);
			}
		}
	}

	if (!fname)
	{
		report(NORM, "ELF file name required");
		return false;
	}

	return true;
}

/**
 * Returns the input ELF file name.
 */
extern const char * 	args_get_input_file_name(void)
{
	return fname;
}


/**
 * Returns the verbosity level (-v -vv options).
 */
extern unsigned int 	args_get_verbosity(void)
{
	return verbosity;
}

/**
 * Returns the naming pattern of the symbols the user is interested in (the -s option).
 */
extern const char * 	args_get_name_pattern(void)
{
	return name_pattern;
}

/**
 * Returns true if only functions should be present in the output (the -f option).
 */
extern bool 		args_get_is_funcs_only(void)
{
	return funcs_only;
}

/**
 * Returns true if offsets are requested to be printed in the decimal format (the -d option).
 */
extern bool 		args_get_is_offsets_decimal(void)
{
	return offsets_decimal;
}

/**
 * Returns true if the symbol name and type satisfy filter specified by the user.
 */
extern bool		args_sym_is_interesting(const char *name, int type)
{
	if (type != STT_FUNC && type != STT_OBJECT)
		return false;

	if (args_get_is_funcs_only() && type != STT_FUNC)
		return false;

	if (args_get_name_pattern() && strstr(name, args_get_name_pattern()) == NULL)
		return false;

	return true;
}
