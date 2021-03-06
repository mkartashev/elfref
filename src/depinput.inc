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

// This file contains bitness-dependent routines for reading and processing
// ELF file. It must be pre-processed before use by replacing $NN with 32 or 64.

#include "depinput.h"
#include "input.h"
#include "errors.h"
#include "symtab.h"
#include "globals.h"
#include "args.h"

#include <stdbool.h>
#include <elf.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>

static const char*	get_sh_str_$NN(input_t* in, uint32_t i)
{
	assert( input_get_elf$NN(in)->shstrtab );

	// The following is not out of range (we checked already)
	const char* sh_strings = &input_get_mem_map(in)[input_get_elf$NN(in)->shstrtab->sh_offset];
	if ( i >= input_get_elf$NN(in)->shstrtab->sh_size )
	{
		fatal("Section header string table index %d out of range (%d)",
			i, input_get_elf$NN(in)->shstrtab->sh_size);
	}

	// our resulting string is always null-terminated (we checked already)
	const char *s = sh_strings+i;
	return s;
}

static const char*	get_str_$NN(input_t* in, void* sec, uint32_t i)
{
	assert( sec );

	Elf$NN_Shdr* strtab = sec;
	// The following is not out of range (we checked already)
	const char* strings = &input_get_mem_map(in)[strtab->sh_offset];
	if ( i >= strtab->sh_size )
	{
		fatal("String table index %d out of range (%d)", i, strtab->sh_size);
	}

	// our resulting string is always null-terminated (we checked already)
	const char* s = strings + i;
	return s;
}

static void	check_sec_size(input_t* in, Elf$NN_Shdr* sec)
{
	if ( (uint64_t)sec->sh_offset + sec->sh_size > input_get_file_size(in) )
	{
		const char* sec_name = get_sh_str_$NN(in, sec->sh_name);
		fatal("section %s goes past end of file (corrupted ELF header?)", sec_name);
	}
}

static void	check_str_sec(input_t* in, Elf$NN_Shdr* sec)
{
	check_sec_size(in, sec);

	if ( input_get_mem_map(in)[sec->sh_offset + sec->sh_size - 1] != 0 )
	{
		const char* s = get_sh_str_$NN(in, sec->sh_name); // could be no null terminator here

		// makes sure sec_name is null-terminated
		static char sec_name[64];
		strncpy(sec_name, s, sizeof(sec_name));
		sec_name[63] = 0;

		fatal("String table section %s not null-terminated (corrupted ELF file?)", sec_name);
	}
}

/**
 * Reads an ELF program header that has different endianness than us and
 * replaces its every data member so that is has the same endianness.
 */
static void	make_ehdr_native_endian_$NN(Elf$NN_Ehdr* ehdr)
{
	assert( ehdr );

	ehdr->e_type = get_uint16(&ehdr->e_type);
	ehdr->e_machine = get_uint16(&ehdr->e_machine);
	ehdr->e_version = get_uint32(&ehdr->e_version);
	ehdr->e_entry = get_uint$NN(&ehdr->e_entry);
	ehdr->e_phoff = get_uint$NN(&ehdr->e_phoff);
	ehdr->e_shoff = get_uint$NN(&ehdr->e_shoff);
	ehdr->e_flags = get_uint32(&ehdr->e_flags);
	ehdr->e_ehsize = get_uint16(&ehdr->e_ehsize);
	ehdr->e_phentsize = get_uint16(&ehdr->e_phentsize);
	ehdr->e_phnum = get_uint16(&ehdr->e_phnum);
	ehdr->e_shentsize = get_uint16(&ehdr->e_shentsize);
	ehdr->e_shnum = get_uint16(&ehdr->e_shnum);
	ehdr->e_shstrndx = get_uint16(&ehdr->e_shstrndx);
}

/**
 * Reads an ELF section header that has different endianness than us and
 * replaces its every data member so that is has the same endianness.
 */
static void	make_shdr_native_endian_$NN(Elf$NN_Shdr* shdr)
{
	assert( shdr );

	shdr->sh_name = get_uint32(&shdr->sh_name);
	shdr->sh_type = get_uint32(&shdr->sh_type);
	shdr->sh_flags = get_uint$NN(&shdr->sh_flags);
	shdr->sh_addr = get_uint$NN(&shdr->sh_addr);
	shdr->sh_offset = get_uint$NN(&shdr->sh_offset);
	shdr->sh_size = get_uint$NN(&shdr->sh_size);
	shdr->sh_link = get_uint32(&shdr->sh_link);
	shdr->sh_info = get_uint32(&shdr->sh_info);
	shdr->sh_addralign = get_uint$NN(&shdr->sh_addralign);
	shdr->sh_entsize = get_uint$NN(&shdr->sh_entsize);
}

static void	find_sym_sec(input_t* in)
{
	Elf$NN_Shdr* sections = input_get_elf$NN(in)->sections;
	int shnum = input_get_elf$NN(in)->shnum;

	for (int i = 0; i < shnum; ++i)
	{
		Elf$NN_Shdr* sec = &sections[i];
		if ( sec->sh_type == SHT_SYMTAB || sec->sh_type == SHT_DYNSYM )
		{
			report(VERB, "Found symtab (%d) and strtab (%d)", i, sec->sh_link);
			if ( sec->sh_link >= (uint32_t)shnum )
			{
				fatal("SYMTAB associated string table index %d out of range (%d)", sec->sh_link, shnum);
			}
			Elf$NN_Shdr* strsec = &sections[sec->sh_link];
			if ( strsec->sh_type != SHT_STRTAB )
			{
				fatal("Type of string table at index %d is not STRTAB", i);
			}

			if( sec->sh_type == SHT_SYMTAB )
			{
				input_get_elf$NN(in)->symtab = sec;
				input_get_elf$NN(in)->strtab = strsec;
				input_get_elf$NN(in)->symtab_idx = i;
			}
			else
			{
				input_get_elf$NN(in)->dsymtab = sec;
				input_get_elf$NN(in)->dstrtab = strsec;
				input_get_elf$NN(in)->dsymtab_idx = i;
			}
		}
	}

	if ( !input_get_elf$NN(in)->symtab && !input_get_elf$NN(in)->dsymtab )
	{
		fatal("No .symtab or .dynsym section in %s", glob_get_program_name());
	}

	// Check the symtab/strtab sections
	if ( input_get_elf$NN(in)->symtab )
	{
		assert(input_get_elf$NN(in)->strtab); // they always go in pairs
		check_sec_size(in, input_get_elf$NN(in)->symtab);
		check_str_sec(in, input_get_elf$NN(in)->strtab);

		// We're going to be reading symtab sequentially real soon
		madvise(&input_get_mem_map(in)[input_get_elf$NN(in)->symtab->sh_offset], input_get_elf$NN(in)->symtab->sh_size, MADV_WILLNEED);
		madvise(&input_get_mem_map(in)[input_get_elf$NN(in)->strtab->sh_offset], input_get_elf$NN(in)->strtab->sh_size, MADV_RANDOM);
	}

	if ( input_get_elf$NN(in)->dsymtab )
	{
		assert(input_get_elf$NN(in)->dstrtab); // they always go in pairs
		check_sec_size(in, input_get_elf$NN(in)->dsymtab);
		check_str_sec(in, input_get_elf$NN(in)->dstrtab);

		// We're going to be reading symtab sequentially real soon
		madvise(&input_get_mem_map(in)[input_get_elf$NN(in)->dsymtab->sh_offset], input_get_elf$NN(in)->dsymtab->sh_size, MADV_WILLNEED);
		madvise(&input_get_mem_map(in)[input_get_elf$NN(in)->dstrtab->sh_offset], input_get_elf$NN(in)->dstrtab->sh_size, MADV_RANDOM);
	}
}

/**
 * Locates all SYMTAB and their corresponding STRTAB sections and stores
 * pointers to them in input_t. Also converts the section headers to
 * the same endianness as us, if necessary.
 */
extern void	find_sections_$NN(input_t* in)
{
	Elf$NN_Ehdr* ehdr = (Elf$NN_Ehdr*)input_get_mem_map(in);
	if ( ! input_get_is_same_endian(in) )
	{
		// Need to modify the program headers in-place in order for us to be able
		// simply read it even though it is of a different endianness
		make_ehdr_native_endian_$NN(ehdr);
	}

	if ( ehdr->e_shoff == 0 )
	{
		fatal("No section info in %s", glob_get_program_name());
	}

	if ( ehdr->e_shoff + (Elf$NN_Off)ehdr->e_shentsize*ehdr->e_shnum > input_get_file_size(in) )
	{
		fatal("Section header table goes past end of file (corrupted ELF header?)");
	}

	input_get_elf$NN(in)->sections = (Elf$NN_Shdr*)&input_get_mem_map(in)[ehdr->e_shoff];
	input_get_elf$NN(in)->shnum = ehdr->e_shnum;

	// Find out about the .shstrtab section:
	if ( ehdr->e_shstrndx == SHN_UNDEF )
	{
		fatal("No .strtab section in %s", glob_get_program_name());
	}

	Elf$NN_Shdr* sections = input_get_elf$NN(in)->sections;
	assert(sections);

	if ( ! input_get_is_same_endian(in) )
	{
		// Need to modify section headers in-place in order for us to be able
		// simply read them even though they are of a different endianness
		for (int i = 0; i < input_get_elf$NN(in)->shnum; ++i)
		{
			make_shdr_native_endian_$NN(&sections[i]);
		}
	}

	if ( ehdr->e_shstrndx >= SHN_LORESERVE )
	{
		if ( ehdr->e_shstrndx != SHN_XINDEX )
		{
			fatal("Bad .shstrtab section index (%x)", ehdr->e_shstrndx);
		}
		// actual index is in sh_link field of the first entry
		if ( sections[0].sh_link >= ehdr->e_shnum )
		{
			fatal(".shstrtab section index (%x) out of range (%d)", sections[0].sh_link, ehdr->e_shnum);
		}
		input_get_elf$NN(in)->shstrtab = &sections[sections[0].sh_link];

		report(VERB, "Found .shstrtab at index %d", sections[0].sh_link);
	}
	else if ( ehdr->e_shstrndx >= ehdr->e_shnum )
	{
		fatal("Out of range .shstrtab section index (corrupted ELF header?)");
	}
	else
	{
		input_get_elf$NN(in)->shstrtab = &sections[ehdr->e_shstrndx];
		report(VERB, "Found .shstrtab at index %d", ehdr->e_shstrndx);
	}

	// Check the .shstrtab section
	check_str_sec(in, input_get_elf$NN(in)->shstrtab);

	// Start reading the section table
	if ( ehdr->e_shentsize != sizeof(Elf$NN_Shdr) )
	{
		fatal("Bad section header size: expected %d, found %d", sizeof(Elf$NN_Shdr), ehdr->e_shentsize);
	}

	find_sym_sec(in);
}

static void	make_sym_same_endian_$NN(Elf$NN_Sym* s)
{
	s->st_name = get_uint32(&s->st_name);
	s->st_value = get_uint$NN(&s->st_value);
	s->st_size = get_uint$NN(&s->st_size);
	s->st_shndx = get_uint16(&s->st_shndx);
}

static size_t	read_symtab_sec(input_t* in,
				Elf$NN_Shdr* symtab,
				Elf$NN_Shdr* strtab,
				symtab_t* syms)
{
	const Elf$NN_Off symsoff = symtab->sh_offset;
	const size_t symtab_nelem = symtab->sh_size / symtab->sh_entsize;
	size_t syms_idx = 0;
	for( size_t i = 0; i < symtab_nelem; ++i)
	{
		size_t symoff = symsoff + i*symtab->sh_entsize;
		Elf$NN_Sym* s = (Elf$NN_Sym*)&input_get_mem_map(in)[symoff];
		if ( !input_get_is_same_endian(in) )
		{
			make_sym_same_endian_$NN(s);
		}

		int symtype = ELF$NN_ST_TYPE(s->st_info);
		size_t symval = s->st_value;
		const char * symname = get_str_$NN(in, strtab, s->st_name);
		syms_idx = symtab_add_sym(syms, symval, symtype, symname);
		report(VERB, "Symbol \"%s\" at index %d", symname, i*symtab->sh_entsize);
	}

	return syms_idx;
}

/**
 * Reads in the input ELF file and returns a pointer to the file's symbol table as symtab_t (see).
 * The returned object must be deallocated with symtab_free().
 */
extern symtab_t*	read_in_symtab_$NN(input_t* in)
{
	size_t nsyms = 0;
	if ( input_get_elf$NN(in)->symtab )
	{
		nsyms += input_get_elf$NN(in)->symtab->sh_size / input_get_elf$NN(in)->symtab->sh_entsize;
	}
	if ( input_get_elf$NN(in)->dsymtab )
	{
		nsyms += input_get_elf$NN(in)->dsymtab->sh_size / input_get_elf$NN(in)->dsymtab->sh_entsize;
	}

	report(VERB, "Found %d symbols total", nsyms);

	symtab_t* symtab = symtab_alloc(nsyms);
	assert(symtab);

	size_t syms_read = 0;
	if ( input_get_elf$NN(in)->symtab )
	{
		syms_read = read_symtab_sec(in, input_get_elf$NN(in)->symtab, input_get_elf$NN(in)->strtab, symtab);
		assert(syms_read <= nsyms);
	}

	if ( input_get_elf$NN(in)->dsymtab )
	{
		syms_read = read_symtab_sec(in, input_get_elf$NN(in)->dsymtab, input_get_elf$NN(in)->dstrtab, symtab);
		assert(syms_read <= nsyms);
	}

	if ( syms_read == 0 )
	{
		report(NORM, "No symbols found in .symtab and .dynsym; nothing to do.");
		return NULL;
	}

	symtab_sort(symtab);

	return symtab;
}

static const char*	get_sym_name_$NN(input_t* in, int symtab_sec_idx, size_t sym_idx, bool *is_func)
{
	// We expect symtab_sec_idx to point to either symtab or dynsym:
	Elf$NN_Shdr* symtab = input_get_elf$NN(in)->symtab;
	Elf$NN_Shdr* strtab = input_get_elf$NN(in)->strtab;
	if ( symtab_sec_idx == input_get_elf$NN(in)->dsymtab_idx )
	{
		symtab = input_get_elf$NN(in)->dsymtab;
		strtab = input_get_elf$NN(in)->dstrtab;
	}
	else if ( symtab_sec_idx != input_get_elf$NN(in)->symtab_idx )
	{
		error("relocation section references unknown symtab (section index %d)", symtab_sec_idx);
		return NULL;
	}

	if ( !symtab || !strtab)
	{
		// May happen when relocation doesn't reference symbols and  only has
		// addends
		return NULL;
	}

	size_t symoff = sym_idx*symtab->sh_entsize;
	if ( symoff >= symtab->sh_size )
	{
		const char* sec_name = get_sh_str_$NN(in, symtab->sh_name);
		error("offset %d into '%s' of symbol index %d is out of range (%d)",
			symoff, sec_name, sym_idx, symtab->sh_size);
	}
	Elf$NN_Sym* s = (Elf$NN_Sym*)&input_get_mem_map(in)[symtab->sh_offset + symoff];
	*is_func = (ELF$NN_ST_TYPE(s->st_info) == STT_FUNC);
	return get_str_$NN(in, strtab, s->st_name);
}

static void	make_rel_same_endian_$NN(Elf$NN_Rel* r)
{
	r->r_offset = get_uint$NN(&r->r_offset);
	r->r_info   = get_uint$NN(&r->r_info);
}

static void	make_rela_same_endian_$NN(Elf$NN_Rela* r)
{
	r->r_offset = get_uint$NN(&r->r_offset);
	r->r_info   = get_uint$NN(&r->r_info);
	r->r_addend = (int$NN_t)get_uint$NN(&r->r_addend);
}

/**
 * Processes relocation records in the given input ELF file, adding information to the given symbol table.
 */
extern void process_relocations_$NN(input_t* in, symtab_t* symtab)
{
	for (int i = 0; i < input_get_elf$NN(in)->shnum; ++i)
	{
		Elf$NN_Shdr* sec = &input_get_elf$NN(in)->sections[i];
		uint32_t typ = sec->sh_type;
		if ( typ == SHT_RELA || typ == SHT_REL )
		{
			madvise(&input_get_mem_map(in)[sec->sh_offset], sec->sh_size, MADV_WILLNEED);
			report(DBG, "Processing rel[a] section \"%s\" at index %d", get_sh_str_$NN(in, sec->sh_name), i);

			uint32_t symtab_sec_idx = sec->sh_link; // this relocation section uses this symtab
			size_t nelem = sec->sh_size / sec->sh_entsize;

			if ( typ == SHT_REL ) // .rel section
			{
				check_sec_size(in, sec);
				Elf$NN_Rel* relocs = (Elf$NN_Rel*)&input_get_mem_map(in)[sec->sh_offset];

				for(size_t j = 0; j < nelem; ++j)
				{
					if ( !input_get_is_same_endian(in) )
					{
						make_rel_same_endian_$NN(&relocs[j]);
					}
					size_t sym_idx = ELF$NN_R_SYM(relocs[j].r_info);
					bool is_func = false;
					const char* sym_name = get_sym_name_$NN(in, (int)symtab_sec_idx, sym_idx, &is_func);
					symtab_add_reloc(symtab, relocs[j].r_offset, sym_name, is_func, 0);
				}
			}
			else  // .rela section
			{
				check_sec_size(in, sec);
				Elf$NN_Rela* relocs = (Elf$NN_Rela*)&input_get_mem_map(in)[sec->sh_offset];

				for(size_t j = 0; j < nelem; ++j)
				{
					if ( !input_get_is_same_endian(in) )
					{
						make_rela_same_endian_$NN(&relocs[j]);
					}
					size_t sym_idx = ELF$NN_R_SYM(relocs[j].r_info);
					bool is_func = false;
					const char* sym_name = get_sym_name_$NN(in, (int)symtab_sec_idx, sym_idx, &is_func);
					symtab_add_reloc(symtab, relocs[j].r_offset, sym_name, is_func, relocs[j].r_addend);
				}
			}
		}
	}
}
