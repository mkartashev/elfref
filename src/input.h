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

typedef	struct symtab_s		symtab_t;
typedef	struct input_s		input_t;
typedef struct elf_sections_s 	elf_sections_t;

input_t *	input_init(void);
void 		input_open(input_t* in);
void		input_close(input_t* in);

// Bitness-independent functions to read input
struct 	reader_funcs
{
	/// Function that locates the necessary sections of the ELF file.
	/// Note: not re-entrant, returns pointer to static memory.
	elf_sections_t * 	(*find_sections)(input_t*);

	/// Functions that reads in symbol tables of the ELF file.
	symtab_t *		(*read_symtab)(input_t*, elf_sections_t* );

	/// Function that reads  in relocation information of the ELF file and updates symtab with it.
	void			(*process_relocations)(input_t*, elf_sections_t*, symtab_t*);
};
struct reader_funcs	input_read_elf_header(input_t* in);

// Input file properties and content access functions
unsigned long long	input_get_file_size(input_t* in);
char *			input_get_mem_map(input_t* in);
bool			input_get_is_same_endian(input_t* in);

// Helper reader functions
uint16_t	get_uint16(void* ptr);
uint32_t	get_uint32(void* ptr);
uint64_t	get_uint64(void* ptr);

#endif

