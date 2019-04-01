# elfref
A tool to find what ELF symbols reference by interpreting relocation records.

This information is useful when debugging a linking issue (usually over email)
and you need to trace the unsatisfied reference back to the source code. The
name of the referrer is not given by all linkers, nor all the customers would
report you that.

## Example
This is how the `elfref` output will look like for a function call in
a C program
```
$ elfref a.o 
elfref: Input (a.o) is a 64-bit little endian ELF relocatable file.
main (addr 0x00000000)
	(+0x001c)-> process_args()-4
```
In a nutshell, this means that the function `main()` refers to 
the function process_args().

## Getting Started
These instructions will get you a copy of the project up and running on your
local machine for development and testing purposes.

### Prerequisites
The build was only tested on Linux (Ubuntu and Red Hat) with gcc 6.+.
Earlier versions of gcc will work, but require minor modification
to `./Makefile` to remove more recent warning options.

### Building
```
$ git clone ... elfref
$ cd elfref/
$ make
```

Issue `make help` for more.

### Usage

Use `elfref -h` to get help:
```
Usage: elfref [OPTIONS]... ELF-FILE
	find what symbols (funcs and global variables) reference in ELF-FILE

Options:
    -s pattern	only show info about symbols of which pattern is a substring
    -f		only show info about functions (symbol type FUNC);
    		by default, OBJECTs are also shown
    -d		print offsets in decimal instead of hex
    -h		display help
    -v		verbose output
    -vv		verbose and debug output

Output format:
sym-name (addr sym-value)    <-- symbol name and address
 (+offset-from-sym) -> [sym-name[()]] [+addend]
 ^                     ^               ^       
 +- offset from sym    |               +- addend (for RELA relocations)
    start              +- name of referenced symbol; () means it's a function
```

## Authors
Maxim Kartashev.

## License
This project is released into the public domain. 
See `LICENSE` for more details.
