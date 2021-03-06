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

#include "input.h"
#include "depinput.h"
#include "errors.h"
#include "globals.h"
#include "args.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

/**
 * Describes the input ELF file, its properties and auxiliary data obtained by parsing its ELF structure.
 */
struct	input_s
{
	int 			fd;		// file descriptor of that file
	unsigned long long 	fsize;

	char * 			map;		// mmap'ed input ELF file

	bool			same_endian;	// input ELF has same endianness as us?
	bool			is_64;		// input ELF is 64-bit?

	union
	{
		struct Elf32	elf32;		// 32-bit ELF file descriptor
		struct Elf64	elf64;		// 64-bit ELF file descriptor
	};
};

/**
 * Returns the size of the input file in bytes.
 */
extern unsigned long long	input_get_file_size(input_t* in)
{
	assert(in);
	return in->fsize;
}

/**
 * Returns the pointer to memory-mapped image of the input file.
 */
extern char *			input_get_mem_map(input_t* in)
{
	assert(in);
	assert(in->map);
	return in->map;
}

/**
 * Returns true if we are the same endianness as the input ELF file.
 */
extern bool			input_get_is_same_endian(input_t* in)
{
	assert(in);
	return in->same_endian;
}

/**
 * Returns pointer to Elf32 (see) that contains ELF-specific information of interest .
 */
extern struct Elf32 *		input_get_elf32(input_t* in)
{
	assert(in);
	assert(!in->is_64);
	return &in->elf32;
}

/**
 * Returns pointer to Elf64 (see) that contains ELF-specific information of interest .
 */
extern struct Elf64 * 		input_get_elf64(input_t* in)
{
	assert(in);
	assert(in->is_64);
	return &in->elf64;
}

/**
 * Returns initialized input.
 */
extern input_t*	input_init(void)
{
	static struct input_s in = {0};

	// Do only non-default init here
	in.fd = -1;

	return &in;
}

/**
 * Opens the input for reading. Issues appropriate errors if they occur during opening.
 */
extern void	input_open(input_t* in)
{
	assert(in);

	in->fd = open(args_get_input_file_name(), O_RDONLY);
	if (in->fd == -1)
	{
		fatal("Cannot open input file (%s)", args_get_input_file_name());
	}

	struct stat sb;
	if (fstat(in->fd, &sb) == -1)
	{
		fatal_err("Cannot stat input file");
	}

	if ((sb.st_mode & S_IFMT) == S_IFDIR)
	{
		fatal("Input can not be a directory");
	}

	if (sb.st_size <= 0)
	{
		fatal("Input file is empty");
	}

	in->fsize = (size_t)sb.st_size; // we know it's not negative, type conv. OK

	// We are going to write to parts of the file to make it easier to access
	// data structures independent of endianness of the input file, so we need
	// write access, but only for our private copy of the pages touched.
	in->map = mmap(NULL, in->fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, in->fd, 0);
	if (in->map == MAP_FAILED)
	{
		fatal_err("Cannot read in input file");
	}
}

/**
 * Releases the resources allocated for the input opened for reading.
 */
extern void	input_close(input_t* in)
{
	assert(in);
	assert(in->map);

	if (munmap(in->map, in->fsize) == -1)
	{
		fatal_err("Cannot unmap input file");
	}

	if (in->fd != -1)
	{
		if (close(in->fd) == -1)
		{
			fatal_err("Cannot close input file");
		}
	}

	in->fd = 0;
	in->map = NULL;
}

/**
 * Returns the set of reader functions appropriate for the input given.
 */
static struct reader_funcs	input_init_reader_funcs(input_t * in)
{
	struct reader_funcs rdr;
	if (in->is_64) // input is 64-bit ELF
	{
		rdr.find_sections = find_sections_64;
		rdr.process_relocations = process_relocations_64;
		rdr.read_symtab = read_in_symtab_64;
	}
	else // input is 32-bit ELF
	{
		rdr.find_sections = find_sections_32;
		rdr.process_relocations = process_relocations_32;
		rdr.read_symtab = read_in_symtab_32;
	}

	return rdr;
}

static const char*	elf_describe(int type)
{
	switch (type)
	{
	case ET_NONE:
		return "none";

	case ET_REL:
		return "relocatable file";

	case ET_EXEC:
		return "executable file";

	case ET_DYN:
		return "shared object";

	case ET_CORE:
		return "core file";

	default:
		fatal("unrecognized object type (corrupted ELF header?)");
		return "none";
	}
}

/**
 * For the opened input, returns the set of functions suitable for reading it.
 */
extern struct reader_funcs	input_read_elf_header(input_t* in)
{
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)in->map; // map must be page-aligned, so no problem with the cast

	// e_ident is the same in either 64- or 32-bit elf; it is also unaffected by endianness
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
	{
		fatal("input not ELF: magic number is different");
	}

	in->is_64 = (ehdr->e_ident[EI_CLASS] == ELFCLASS64);

	const bool input_big_endian = ehdr->e_ident[EI_DATA] == ELFDATA2MSB;
	const bool host_big_endian = (((union {unsigned int i; char c; }){1}).c) == 0;
	in->same_endian = (input_big_endian == host_big_endian);

	int typ = in->same_endian ? ehdr->e_type : get_uint16(&ehdr->e_type);
	const char *descr = elf_describe(typ);
	report(NORM, "Input (%s) is a %s-bit %s endian ELF %s.",
	       args_get_input_file_name(),
	       in->is_64 ? "64" : "32",
	       input_big_endian ? "big" : "little",
	       descr);

	return input_init_reader_funcs(in);
}

////////////////////// Input file reader functions ////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/**
 * Returns a 64-bit value pointed to by vp interpreting the bytes as having a different endianness than this program.
 */
extern uint64_t		get_uint64(void* vp)
{
	char *p = vp;
	union
	{
		char c[8];
		uint64_t i;
	} u;

	for (size_t j = 0; j < 8; ++j)
	{
		u.c[7 - j] = *(p + j);
	}

	return u.i;
}

/**
 * Returns a 32-bit value pointed to by vp interpreting the bytes as having a different endianness than this program.
 */
extern uint32_t		get_uint32(void* vp)
{
	char *p = vp;
	union
	{
		char c[4];
		uint32_t i;
	} u;
	u.c[3] = *(p + 0);
	u.c[2] = *(p + 1);
	u.c[1] = *(p + 2);
	u.c[0] = *(p + 3);
	return u.i;
}

/**
 * Returns a 16-bit value pointed to by vp interpreting the bytes as having a different endianness than this program.
 */
extern uint16_t		get_uint16(void* vp)
{
	char *p = vp;
	union
	{
		char c[2];
		uint16_t i;
	} u;
	u.c[1] = *(p + 0);
	u.c[0] = *(p + 1);
	return u.i;
}
