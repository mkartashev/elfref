
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

#ifndef DEPINPUT_H_
#define DEPINPUT_H_

typedef struct input_s		input_t;
typedef struct symtab_s		symtab_t;
typedef struct elf_sections_s 	elf_sections_t;

// Bitness-dependent versions, implementations are in depintpu[32|64].c, which is produced by pre-processing depinput.inc
elf_sections_t *	find_sections_32(input_t* in);
symtab_t*		read_in_symtab_32(input_t* in, elf_sections_t*);
void			process_relocations_32(input_t* in, elf_sections_t*, symtab_t*);

elf_sections_t*		find_sections_64(input_t* in);
symtab_t*		read_in_symtab_64(input_t* in, elf_sections_t*);
void			process_relocations_64(input_t* in, elf_sections_t*, symtab_t*);

#endif // DEPINPUT_H_