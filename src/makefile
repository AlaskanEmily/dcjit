# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

EXT?=
SO?=.so

all: dc$(EXT) dc$(SO)

SYSFLAGS?=-g
SYSFLAGS+=-fPIC
LINKFLAGS?=
LINKFLAGS+=$(SYSFLAGS)
CCFLAGS=-Wall -Wextra -pedantic -Werror -Os $(SYSFLAGS)
CFLAGS=$(CCFLAGS) -ansi -Wenum-compare -Wshadow
CXXFLAGS=$(CCFLAGS) -std=c++98 -fno-rtti -fno-exceptions

CC?=gcc
CXX?=g++
AR?=ar
RANLIB?=ranlib

ARCH?=x86
PLATFORM?=unix
YASMOBJ?=elf
BACKEND?=$(ARCH)_$(PLATFORM)
#BACKEND?=soft

dc_main.o: dc_main.c dc.h
	$(CC) $(CFLAGS) -c dc_main.c -o dc_main.o

dc_core.o: dc_core.c dc.h dc_backend.h
	$(CC) $(CFLAGS) -c dc_core.c -o dc_core.o

# Soft components
dc_soft.o: dc_soft.cpp dc_backend.h
	$(CXX) $(CXXFLAGS) -c dc_soft.cpp -o dc_soft.o

libdcjit_soft.a: dc_soft.o
	$(AR) rc libdcjit_soft.a dc_soft.o
	$(RANLIB) libdcjit_soft.a

# JIT platform components
dc_jit_unix.o: dc_jit_unix.c dc_jit.h
	$(CC) $(CFLAGS) -c dc_jit_unix.c -o dc_jit_unix.o

dc_jit_haiku.o: dc_jit_haiku.cpp dc_jit.h
	$(CXX) $(CXXFLAGS) -c dc_jit_haiku.cpp -o dc_jit_haiku.o

dc_jit_win32.o: dc_jit_win32.c dc_jit.h
	$(CC) $(CFLAGS) -c dc_jit_win32.c -o dc_jit_win32.o

# This object provides calling convention conversions from the Win64 ABI to
# the SysV ABI calling convention that dc_jit_amd64 implements.
dc_jit_win64.o: dc_jit_win64.s
	yasm -f $(YASMOBJ)64 -Worphan-labels dc_jit_win64.s -o dc_jit_win64.o

# JIT arch components
dc_jit_x86.o: dc_jit_x86.s
	yasm -f $(YASMOBJ)32 -Worphan-labels dc_jit_x86.s -o dc_jit_x86.o

dc_jit_amd64.o: dc_jit_amd64.s
	yasm -f $(YASMOBJ)64 -Worphan-labels dc_jit_amd64.s -o dc_jit_amd64.o

# General JIT components
dc_jit.o: dc_jit.c dc_jit.h dc_backend.h
	$(CC) $(CFLAGS) -c dc_jit.c -o dc_jit.o

# Unix
X86_OBJECTS=dc_jit_x86.o
AMD64_OBJECTS=dc_jit_amd64.o
UNIX_OBJECTS=dc_jit_unix.o
HAIKU_OBJECTS=dc_jit_haiku.o
WIN32_OBJECTS=dc_jit_win32.o
WIN64_OBJECTS=dc_jit_win64.o $(WIN32_OBJECTS)
COMMON_OBJECTS=dc_jit.o

# Unix
libdcjit_x86_unix.a: $(X86_OBJECTS) $(UNIX_OBJECTS) $(COMMON_OBJECTS)
	$(AR) rc libdcjit_x86_unix.a $(X86_OBJECTS) $(UNIX_OBJECTS) $(COMMON_OBJECTS)
	$(RANLIB) libdcjit_x86_unix.a

libdcjit_amd64_unix.a: $(AMD64_OBJECTS) $(UNIX_OBJECTS) $(COMMON_OBJECTS)
	$(AR) rc libdcjit_amd64_unix.a $(AMD64_OBJECTS) $(UNIX_OBJECTS) $(COMMON_OBJECTS)
	$(RANLIB) libdcjit_amd64_unix.a

# Windows
libdcjit_x86_win32.a: $(X86_OBJECTS) $(WIN32_OBJECTS) $(COMMON_OBJECTS)
	$(AR) rc libdcjit_x86_win32.a $(AMD64_OBJECTS) $(WIN32_OBJECTS) $(COMMON_OBJECTS)
	$(RANLIB) libdcjit_x86_win32.a

libdcjit_amd64_win64.a: $(AMD64_OBJECTS) $(WIN64_OBJECTS) $(COMMON_OBJECTS)
	$(AR) rc libdcjit_amd64_win64.a $(AMD64_OBJECTS) $(WIN64_OBJECTS) $(COMMON_OBJECTS)
	$(RANLIB) libdcjit_amd64_win64.a

# Haiku
libdcjit_x86_haiku.a: $(X86_OBJECTS) $(HAIKU_OBJECTS) $(COMMON_OBJECTS)
	$(AR) rc libdcjit_x86_haiku.a $(X86_OBJECTS) $(HAIKU_OBJECTS) $(COMMON_OBJECTS)
	$(RANLIB) libdcjit_x86_haiku.a

# Emscripten components
dc_js.o: dc_js.cpp dc_backend.h
	$(CXX) $(CXXFLAGS) -std=c++14 -c dc_js.cpp -o dc_js.o

libdcjit_js.a: dc_js.o
	$(AR) rc libdcjit_js.a dc_js.o
	$(RANLIB) libdcjit_js.a

OBJECTS=dc_main.o dc_core.o

dc$(SO): $(OBJECTS) libdcjit_$(BACKEND).a
	$(CXX) $(LINKFLAGS) -shared dc_core.o libdcjit_$(BACKEND).a -o dc$(SO)

dc$(EXT): $(OBJECTS) libdcjit_$(BACKEND).a
	$(CXX) $(LINKFLAGS) $(OBJECTS) libdcjit_$(BACKEND).a -o dc$(EXT)

emscripten: dc$(SO)
	cat dc_jit_js.js dc$(SO) > libdc.js

clean:
	rm *.o *.a

