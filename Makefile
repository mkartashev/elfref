#  This is free and unencumbered software released into the public domain.
#
#  Anyone is free to copy, modify, publish, use, compile, sell, or
#  distribute this software, either in source code form or as a compiled
#  binary, for any purpose, commercial or non-commercial, and by any
#  means.
#
#  In jurisdictions that recognize copyright laws, the author or authors
#  of this software dedicate any and all copyright interest in the
#  software to the public domain. We make this dedication for the benefit
#  of the public at large and to the detriment of our heirs and
#  successors. We intend this dedication to be an overt act of
#  relinquishment in perpetuity of all present and future rights to this
#  software under copyright law.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#  OTHER DEALINGS IN THE SOFTWARE.
#
#  For more information, please refer to <http://unlicense.org/>

SUBDIRS = src

# Compiler flags common to all build modes
CFLAGS_comm := -std=c11 -m64
CFLAGS_comm += -Wall -Wextra -Werror
CFLAGS_comm += -Wformat-security -Wduplicated-cond -Wfloat-equal -Wshadow -Wconversion -Wjump-misses-init -Wlogical-not-parentheses -Wnull-dereference
CFLAGS_comm += -D_GNU_SOURCE

# Mode-specific compiler flags
CFLAGS_debug := -g -O1
CFLAGS_opt   := -O -flto

# The default mode
MODE = opt

CFLAGS = $(CFLAGS_$(MODE))
CFLAGS+= $(CFLAGS_comm)

HDR :=
SRC := Makefile src/Makefile
OBJ :=
GEN_SRC := 
BIN := elfref

.PHONY: clean clobber tar help

all: $(BIN)

help:
	@echo "Targets:"
	@echo "    all     - build optimized version (default)"
	@echo "    tar     - package source and makefiles"
	@echo "    clean   - remove object and generated source files"
	@echo "	   clobber - clean and remove auto-generated dependencies"
	@echo
	@echo "Options:"
	@echo "    MODE=opt   - build optimized version (default)"
	@echo "        =debug - build debug version"

include $(patsubst %,%/Makefile,$(SUBDIRS))

DEPS = $(OBJ:.o=.d)
INCLUDES = $(patsubst %,-I %/,$(SUBDIRS))

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJ)

clean:
	$(RM) $(BIN)
	$(RM) $(OBJ)
	$(RM) $(GEN_SRC)

clobber: clean
	$(RM) $(DEPS)

tar:
	@tar czvf elfrefs.tar.gz $(SRC) $(HDR) src/depinput.inc > /dev/null
	@echo "Source tree packed into elfrefs.tar.gz"

obj/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.d: src/%.c
	@mkdir -p ./obj/
	$(CC) $(CFLAGS) -MT obj/$*.o -MM $< > $@

-include $(DEPS)

