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
// ELF file. It must be pre-processed before use by replacing $NN with 32 or 64
#include "input.h"
#include "errors.h"
#include "symtab.h"
#include "globals.h"

#include <stdbool.h>
#include <elf.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>

const char* get_sh_str_$NN(struct input* in, uint32_t i)
{
    assert( in->map );
    assert( in->elf$NN.shstrtab );

    // The following is not out of range (we checked already)
    const char* sh_strings = &in->map[in->elf$NN.shstrtab->sh_offset];
    if ( i >= in->elf$NN.shstrtab->sh_size ) {
	fatal("Section header string table index %d out of range (%d)", 
		i, in->elf$NN.shstrtab->sh_size);
    }

    // our resulting string is always null-terminated (we checked already)
    const char *s = sh_strings+i;
    return s;
}

const char* get_str_$NN(struct input* in, void* sec, uint32_t i) 
{
    assert( in->map );
    assert( sec );

    Elf$NN_Shdr* strtab = sec;
    // The following is not out of range (we checked already)
    const char* strings = &in->map[strtab->sh_offset];
    if ( i >= strtab->sh_size ) {
	fatal("String table index %d out of range (%d)", i, strtab->sh_size);
    }

    // our resulting string is always null-terminated (we checked already)
    const char* s = strings + i;
    return s;
}

static void check_sec_size(struct input* in, Elf$NN_Shdr* sec)
{
    if ( (uint64_t)sec->sh_offset + sec->sh_size > in->fsize ) {
	const char* sec_name = get_sh_str_$NN(in, sec->sh_name);
	fatal("section %s goes past end of file (corrupted ELF header?)", sec_name);
    }
}

static void check_str_sec(struct input* in, Elf$NN_Shdr* sec)
{
    check_sec_size(in, sec);

    if ( in->map[sec->sh_offset + sec->sh_size - 1] != 0 ) {
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
void make_ehdr_native_endian_$NN(Elf$NN_Ehdr* ehdr)
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
void make_shdr_native_endian_$NN(Elf$NN_Shdr* shdr)
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

static void find_sym_sec(struct input* in)
{
    Elf$NN_Shdr* sections = in->elf$NN.sections;
    int shnum = in->elf$NN.shnum;

    for (int i = 0; i < shnum; ++i) {
	Elf$NN_Shdr* sec = &sections[i];
	if ( sec->sh_type == SHT_SYMTAB || sec->sh_type == SHT_DYNSYM ) {
	    report(VERB, "Found symtab (%d) and strtab (%d)", i, sec->sh_link);
	    if ( sec->sh_link >= (uint32_t)shnum ) {
		fatal("SYMTAB associated string table index %d out of range (%d)", 
			sec->sh_link, shnum);
	    }
	    Elf$NN_Shdr* strsec = &sections[sec->sh_link]; 
	    if ( strsec->sh_type != SHT_STRTAB ) {
		fatal("Type of string table at index %d is not STRTAB", i);
	    }

	    if( sec->sh_type == SHT_SYMTAB ) {
		in->elf$NN.symtab = sec;
		in->elf$NN.strtab = strsec;
		in->elf$NN.symtab_idx = i;
	    } else {
		in->elf$NN.dsymtab = sec;
		in->elf$NN.dstrtab = strsec;
		in->elf$NN.dsymtab_idx = i;
	    }
	} 
    }

    if ( !in->elf$NN.symtab && !in->elf$NN.dsymtab ) {
	fatal("No .symtab or .dynsym section in %s", globals.opts.fname);
    }

    // Check the symtab/strtab sections
    if ( in->elf$NN.symtab ) {
	assert(in->elf$NN.strtab); // they always go in pairs
	check_sec_size(in, in->elf$NN.symtab);
	check_str_sec(in, in->elf$NN.strtab);

	// We're going to be reading symtab sequentially real soon
	madvise(&in->map[in->elf$NN.symtab->sh_offset], in->elf$NN.symtab->sh_size, MADV_WILLNEED);
	madvise(&in->map[in->elf$NN.strtab->sh_offset], in->elf$NN.strtab->sh_size, MADV_RANDOM);
    }

    if ( in->elf$NN.dsymtab ) {
	assert(in->elf$NN.dstrtab); // they always go in pairs
	check_sec_size(in, in->elf$NN.dsymtab);
	check_str_sec(in, in->elf$NN.dstrtab);

	// We're going to be reading symtab sequentially real soon
	madvise(&in->map[in->elf$NN.dsymtab->sh_offset], in->elf$NN.dsymtab->sh_size, MADV_WILLNEED);
	madvise(&in->map[in->elf$NN.dstrtab->sh_offset], in->elf$NN.dstrtab->sh_size, MADV_RANDOM);
    }
}
/**
 * Locates all SYMTAB and their corresponding STRTAB sections and stores 
 * pointers to them in struct input. Also converts the section headers to
 * the same endianness as us, if necessary.
 */
void find_sections_$NN(struct input* in)
{
    Elf$NN_Ehdr* ehdr = (Elf$NN_Ehdr*)in->map;
    if ( ! in->same_endian ) {
	// Need to modify the program headers in-place in order for us to be able
	// simply read it even though it is of a different endianness
	make_ehdr_native_endian_$NN(ehdr);
    }

    if ( ehdr->e_shoff == 0 ) {
	fatal("No section info in %s", globals.opts.fname);
    }
    
    if ( ehdr->e_shoff + (Elf$NN_Off)ehdr->e_shentsize*ehdr->e_shnum > in->fsize ) {
	fatal("Section header table goes past end of file (corrupted ELF header?)");
    }

    in->elf$NN.sections = (Elf$NN_Shdr*)&in->map[ehdr->e_shoff];
    in->elf$NN.shnum = ehdr->e_shnum;

    // Find out about the .shstrtab section:
    if ( ehdr->e_shstrndx == SHN_UNDEF ) {
	fatal("No .strtab section in %s", globals.opts.fname);
    }

    Elf$NN_Shdr* sections = in->elf$NN.sections;
    assert(sections);

    if ( ! in->same_endian ) {
	// Need to modify section headers in-place in order for us to be able
	// simply read them even though they are of a different endianness
	for (int i = 0; i < in->elf$NN.shnum; ++i) {
	    make_shdr_native_endian_$NN(&sections[i]);
	}
    }

    if ( ehdr->e_shstrndx >= SHN_LORESERVE ) {
	if ( ehdr->e_shstrndx != SHN_XINDEX ) {
	    fatal("Bad .shstrtab section index (%x)", ehdr->e_shstrndx);
	}
	// actual index is in sh_link field of the first entry
	if ( sections[0].sh_link >= ehdr->e_shnum ) {
	    fatal(".shstrtab section index (%x) out of range (%d)", sections[0].sh_link, ehdr->e_shnum);
	}
	in->elf$NN.shstrtab = &sections[sections[0].sh_link];

	report(VERB, "Found .shstrtab at index %d", sections[0].sh_link);
    } else if ( ehdr->e_shstrndx >= ehdr->e_shnum ) {
	fatal("Out of range .shstrtab section index (corrupted ELF header?)");
    } else {
	in->elf$NN.shstrtab = &sections[ehdr->e_shstrndx];
	report(VERB, "Found .shstrtab at index %d", ehdr->e_shstrndx);
    }

    // Check the .shstrtab section
    check_str_sec(in, in->elf$NN.shstrtab);

    // Start reading the section table
    if ( ehdr->e_shentsize != sizeof(Elf$NN_Shdr) ) {
	fatal("Bad section header size: expected %d, found %d", sizeof(Elf$NN_Shdr), ehdr->e_shentsize);
    }

    find_sym_sec(in);
}

void make_sym_same_endian_$NN(Elf$NN_Sym* s)
{
    s->st_name = get_uint32(&s->st_name);
    s->st_value = get_uint$NN(&s->st_value);
    s->st_size = get_uint$NN(&s->st_size);
    s->st_shndx = get_uint16(&s->st_shndx);
}

static size_t read_symtab_sec(struct input* in, 
			    Elf$NN_Shdr* symtab, 
			    Elf$NN_Shdr* strtab,
			    sym* syms,  // array of sym...
			    size_t nelem, // of nelem elements
			    size_t start_idx) // fill it starting from this index
{
    const Elf$NN_Off symsoff = symtab->sh_offset;
    const size_t symtab_nelem = symtab->sh_size / symtab->sh_entsize;
    size_t syms_idx = start_idx;
    for( size_t i = 0; i < symtab_nelem; ++i) {
	size_t symoff = symsoff + i*symtab->sh_entsize;
	Elf$NN_Sym* s = (Elf$NN_Sym*)&in->map[symoff];
	if ( !in->same_endian ) {
	    make_sym_same_endian_$NN(s);
	}

	const char* name = get_str_$NN(in, strtab, s->st_name);
	int typ = ELF$NN_ST_TYPE(s->st_info);
	if ( sym_is_interesting(name, typ) ) {
	    assert(syms_idx < nelem);

	    syms[syms_idx].offset = s->st_value;
	    syms[syms_idx].name   = get_str_$NN(in, strtab, s->st_name);
	    report(VERB, "Symbol \"%s\" at index %d", syms[syms_idx].name, i*symtab->sh_entsize);
	    syms_idx++;
	}
    }

    return syms_idx;
}

struct symtab* read_in_symtab_$NN(struct input* in)
{
    size_t nsyms = 0;
    if ( in->elf$NN.symtab ) {
	nsyms += in->elf$NN.symtab->sh_size / in->elf$NN.symtab->sh_entsize;
    }
    if ( in->elf$NN.dsymtab ) {
	nsyms += in->elf$NN.dsymtab->sh_size / in->elf$NN.dsymtab->sh_entsize;
    }

    report(VERB, "Found %d symbols total", nsyms);

    struct symtab* symtab = alloc_symtab(nsyms);
    assert(symtab);

    size_t syms_read = 0;
    if ( in->elf$NN.symtab ) {
	syms_read = read_symtab_sec(in, in->elf$NN.symtab, 
				   in->elf$NN.strtab, symtab->syms, nsyms, 0);
        assert(syms_read < nsyms);
    }

    if ( in->elf$NN.dsymtab ) {
	syms_read = read_symtab_sec(in, in->elf$NN.dsymtab, in->elf$NN.dstrtab, 
				   symtab->syms, nsyms, syms_read);
       assert(syms_read < nsyms);
    }

    symtab->nsyms = syms_read; // we need to remember the actual number of syms in symtab

    if ( syms_read == 0 ) {
	if ( globals.opts.funcs_only || globals.opts.name_pattern ) {
	    report(NORM, "No symbols that match pattern found in .symtab and .dynsym; nothing to do.");
	} else {
	    report(NORM, "No symbols found in .symtab and .dynsym; nothing to do.");
	}
	return NULL;
    }

    sort_symtab(symtab);
    dump_symtab(symtab);

    return symtab;
}

const char* get_sym_name_$NN(struct input* in, int symtab_sec_idx, size_t sym_idx, bool *is_func)
{
    // We expect symtab_sec_idx to point to either symtab or dynsym:
    Elf$NN_Shdr* symtab = in->elf$NN.symtab;
    Elf$NN_Shdr* strtab = in->elf$NN.strtab;
    if ( symtab_sec_idx == in->elf$NN.dsymtab_idx ) {
	symtab = in->elf$NN.dsymtab;
	strtab = in->elf$NN.dstrtab;
    } else if ( symtab_sec_idx != in->elf$NN.symtab_idx ) {
	error("relocation section references unknown symtab (section index %d)", symtab_sec_idx);
	return NULL;
    }

    if ( !symtab || !strtab) {
	// May happen when relocation doesn't reference symbols and  only has
	// addends
	return NULL;
    }

    size_t symoff = sym_idx*symtab->sh_entsize;
    if ( symoff >= symtab->sh_size ) {
	const char* sec_name = get_sh_str_$NN(in, symtab->sh_name);
	error("offset %d into '%s' of symbol index %d is out of range (%d)", 
	    symoff, sec_name, sym_idx, symtab->sh_size);
    }
    Elf$NN_Sym* s = (Elf$NN_Sym*)&in->map[symtab->sh_offset + symoff];
    *is_func = (ELF$NN_ST_TYPE(s->st_info) == STT_FUNC);
    return get_str_$NN(in, strtab, s->st_name);
}

void make_rel_same_endian_$NN(Elf$NN_Rel* r)
{
    r->r_offset = get_uint$NN(&r->r_offset);
    r->r_info   = get_uint$NN(&r->r_info);
}

void make_rela_same_endian_$NN(Elf$NN_Rela* r)
{
    r->r_offset = get_uint$NN(&r->r_offset);
    r->r_info   = get_uint$NN(&r->r_info);
    r->r_addend = (int$NN_t)get_uint$NN(&r->r_addend);
}

void process_relocations_$NN(struct input* in, struct symtab* symtab)
{
    for (int i = 0; i < in->elf$NN.shnum; ++i) {
	Elf$NN_Shdr* sec = &in->elf$NN.sections[i]; 
	uint32_t typ = sec->sh_type;
	if ( typ == SHT_RELA || typ == SHT_REL ) {
	    madvise(&in->map[sec->sh_offset], sec->sh_size, MADV_WILLNEED);
	    report(DBG, "Processing rel[a] section \"%s\" at index %d", get_sh_str_$NN(in, sec->sh_name), i);

	    uint32_t symtab_sec_idx = sec->sh_link; // this relocation section uses this symtab
	    size_t nelem = sec->sh_size / sec->sh_entsize;

	    if ( typ == SHT_REL ) { // .rel section
		check_sec_size(in, sec);
		Elf$NN_Rel* relocs = (Elf$NN_Rel*)&in->map[sec->sh_offset];

		for(size_t j = 0; j < nelem; ++j) {
		    if ( !in->same_endian ) {
			make_rel_same_endian_$NN(&relocs[j]);
		    }
		    size_t sym_idx = ELF$NN_R_SYM(relocs[j].r_info);
		    bool is_func = false;
		    const char* sym_name = get_sym_name_$NN(in, (int)symtab_sec_idx, sym_idx, &is_func);
		    add_reloc(symtab, relocs[j].r_offset, sym_name, is_func, 0);
		}
	    } else { // .rela section
		check_sec_size(in, sec);
		Elf$NN_Rela* relocs = (Elf$NN_Rela*)&in->map[sec->sh_offset];
		
		for(size_t j = 0; j < nelem; ++j) {
		    if ( !in->same_endian ) {
			make_rela_same_endian_$NN(&relocs[j]);
		    }
		    size_t sym_idx = ELF$NN_R_SYM(relocs[j].r_info);
		    bool is_func = false;
		    const char* sym_name = get_sym_name_$NN(in, (int)symtab_sec_idx, sym_idx, &is_func);
		    add_reloc(symtab, relocs[j].r_offset, sym_name, is_func, relocs[j].r_addend);
		}
	    }
	}
    }
}

