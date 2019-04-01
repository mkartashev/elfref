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

#ifndef INPUT_H_
#define INPUT_H_
#include <stdbool.h>
#include <elf.h>
#include <stdlib.h>
#include <sys/types.h>

struct Elf32 
{
    Elf32_Shdr* sections; // array of all sections
    uint16_t	shnum;    // number of elements in that array

    Elf32_Shdr* shstrtab; // string table for section names

    // symtab
    Elf32_Shdr* symtab;	    // the .symtab section
    int		symtab_idx; // the section's index
    Elf32_Shdr* strtab;	    // corresponding string section for symbol names

    // dynsym
    Elf32_Shdr* dsymtab;    // the .dynsym section
    int		dsymtab_idx;// the section's index
    Elf32_Shdr* dstrtab;    // corresponding string section for symbol names
} elf32;

struct Elf64 
{
    Elf64_Shdr* sections;
    uint16_t	shnum;

    Elf64_Shdr* shstrtab;

    // symtab
    Elf64_Shdr* symtab;
    int		symtab_idx;
    Elf64_Shdr* strtab;

    // dynsym
    Elf64_Shdr* dsymtab;
    int		dsymtab_idx;
    Elf64_Shdr* dstrtab;
} elf64;

struct input;
struct symtab;

struct reader_funcs
{
    // Bitness independent functions to read input
    void (*find_sections)(struct input*);
    struct symtab* (*read_symtab)(struct input*);
    void (*process_relocations)(struct input*, struct symtab*);
};

struct input
{
    int fd;			// file descriptor of that file
    unsigned long long fsize;

    char* map;			// mmap'ed input ELF file

    bool same_endian;		// input ELF has same endianness as us?
    bool is_64;			// input ELF is 64-bit?

    union
    {
	struct Elf32 elf32;
	struct Elf64 elf64;
    };
};

void init_input(struct input*);
void open_input(struct input*);
void fini_input(struct input*);
struct reader_funcs init_reader_funcs(struct input*);

uint16_t get_uint16(void* ptr);
uint32_t get_uint32(void* ptr);
uint64_t get_uint64(void* ptr);

struct reader_funcs read_elf_header(struct input* in);
void find_sections_32(struct input* in);
void process_relocations_32(struct input* in, struct symtab*);
struct symtab* read_in_symtab_32(struct input* in);

void find_sections_64(struct input* in);
void process_relocations_64(struct input* in, struct symtab*);
struct symtab* read_in_symtab_64(struct input* in);

// Filtering function for symtab
bool sym_is_interesting(const char* name, int type);
#endif

