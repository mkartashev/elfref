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
#include "errors.h"
#include "globals.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

void init_input(struct input* in)
{
    assert( in );

    *in = (struct input){0};

    // Do only non-default init here
    in->fd = -1;
}

void open_input(struct input* in)
{
    assert( in );

    in->fd = open(globals.opts.fname, O_RDONLY);
    if (in->fd == -1) {
	fatal_err("Cannot open input file");
    }

    struct stat sb;
    if ( fstat(in->fd, &sb) == -1 ) {
	fatal_err("Cannot stat input file");
    }

    if ( (sb.st_mode & S_IFMT) == S_IFDIR ) {
	fatal("Input can not be a directory");
    }

    if ( sb.st_size <= 0 ) {
	fatal("Input file is empty");
    }

    in->fsize = (size_t)sb.st_size; // we know it's not negative, type conv. OK

    // We are going to write to parts of the file to make it easier to access
    // data structures independent of endianness of the input file, so we need
    // write access, but only for our private copy of the pages touched.
    in->map = mmap(NULL, in->fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, in->fd, 0);
    if ( in->map == MAP_FAILED ) {
	fatal_err("Cannot read in input file");
    }
}

void fini_input(struct input* in)
{
    assert( in );

    if ( munmap(in->map, in->fsize) == -1 ) {
	fatal_err("Cannot unmap input file");
    }

    if ( in->fd != -1 ) {
	if ( close(in->fd) == -1 ) {
	    fatal_err("Cannot close input file");
	}
    }
}

////////////////////// Input file reader functions ////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint64_t get_uint64(void* vp)
{
    char* p = vp;
    union {
	char c[8];
	uint64_t i;
    } u;

    for (size_t j = 0; j < 8; ++j) {
	u.c[7-j] =  *(p + j);
    }

    return u.i;
}

uint32_t get_uint32(void* vp)
{
    char* p = vp;
    union {
	char c[4];
	uint32_t i;
    } u;
    u.c[3] = *(p+0);
    u.c[2] = *(p+1);
    u.c[1] = *(p+2);
    u.c[0] = *(p+3);
    return u.i;
}

uint16_t get_uint16(void* vp)
{
    char* p = vp;
    union {
	char c[2];
	uint16_t i;
    } u;
    u.c[1] = *(p+0);
    u.c[0] = *(p+1);
    return u.i;
}


///////////////////////////////////////////////////////////////////////////////
struct reader_funcs init_reader_funcs(struct input* in)
{
    struct reader_funcs rdr;
    if ( in->is_64 ) { // input is 64-bit ELF
	rdr.find_sections = find_sections_64;
	rdr.process_relocations = process_relocations_64;
	rdr.read_symtab = read_in_symtab_64;
    } else { // input is 32-bit ELF
	rdr.find_sections = find_sections_32;
	rdr.process_relocations = process_relocations_32;
	rdr.read_symtab = read_in_symtab_32;
    }

    return rdr;
}

/**
 * Returns true if the symbol name and type satisfy filter specified by the user
 */
bool sym_is_interesting(const char* name, int type)
{
     if ( type != STT_FUNC && type != STT_OBJECT)
	 return false;

     if ( globals.opts.funcs_only && type != STT_FUNC )
	 return false;

     if ( globals.opts.name_pattern && strstr(globals.opts.name_pattern, name) == NULL )
	 return false;

     return true;
}

const char* elf_describe(int type)
{
    switch( type ) {
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

struct reader_funcs read_elf_header(struct input* in)
{
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)in->map; // map must be page-aligned, so no problem with the cast

    // e_ident is the same in either 64- or 32-bit elf; it is also unaffected
    // by endianness
    if ( ehdr->e_ident[EI_MAG0] != ELFMAG0 
	    || ehdr->e_ident[EI_MAG1] != ELFMAG1
	    || ehdr->e_ident[EI_MAG2] != ELFMAG2 
	    || ehdr->e_ident[EI_MAG3] != ELFMAG3 ) {
	fatal("input not ELF: magic number is different");
    }

    in->is_64 = (ehdr->e_ident[EI_CLASS] == ELFCLASS64);

    const bool input_big_endian = ehdr->e_ident[EI_DATA] == ELFDATA2MSB;
    const bool host_big_endian = (((union {unsigned int i; char c;}){1}).c) == 0;
    in->same_endian = (input_big_endian == host_big_endian);

    int typ = in->same_endian ? ehdr->e_type : get_uint16(&ehdr->e_type);
    const char* descr = elf_describe(typ);
    report(NORM, "Input (%s) is a %s-bit %s endian ELF %s.", 
	    globals.opts.fname, 
	    in->is_64 ? "64" : "32", 
	    input_big_endian ? "big" : "little",
	    descr);

    return init_reader_funcs(in); 
}

