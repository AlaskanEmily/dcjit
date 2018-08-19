# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

all: dc

SYSFLAGS=-g
LINKFLAGS=$(SYSFLAGS)
CCFLAGS=-Wall -Wextra -pedantic -Werror -Os $(SYSFLAGS)
CFLAGS=$(CCFLAGS) -ansi -Wenum-compare -Wshadow
CXXFLAGS=$(CCFLAGS) -std=c++98 -fno-rtti -fno-exceptions

CC?=gcc
CXX?=g++

BACKEND?=soft
ARCH?=x86
PLATFORM?=unix

dc_main.o: dc_main.c dc.h
	$(CC) $(CFLAGS) -c dc_main.c -o dc_main.o

dc_core.o: dc_core.c dc.h dc_backend.h
	$(CC) $(CFLAGS) -c dc_core.c -o dc_core.o

# Soft components
dc_soft.o: dc_soft.cpp dc_backend.h
	$(CXX) $(CXXFLAGS) -c dc_soft.cpp -o dc_soft.o

libdcjit_soft.a: dc_soft.o
	ar rc libdcjit_soft.a dc_soft.o
	ranlib libdcjit_soft.a

# JIT platform components
dc_jit_unix.o: dc_jit_unix.c dc_jit.h
	$(CC) $(CFLAGS) -c dc_jit_unix.c -o dc_jit_unix.o

# JIT arch components
dc_jit_x86.o: dc_jit_x86.s
	yasm -f elf32 -Worphan-labels dc_jit_x86.s

dc_jit_amd64.o: dc_jit_amd64.s
	yasm -f elf64 -Worphan-labels dc_jit_amd64.s

# General JIT components
dc_jit.o: dc_jit.c dc_jit.h dc_backend.h
	$(CC) $(CFLAGS) -c dc_jit.c -o dc_jit.o

libdcjit_x86_unix.a: dc_jit_x86.o dc_jit_unix.o dc_jit.o
	ar rc libdcjit_x86_unix.a dc_jit_x86.o dc_jit_unix.o dc_jit.o
	ranlib libdcjit_x86_unix.a

OBJECTS=dc_main.o dc_core.o

dc: $(OBJECTS) libdcjit_$(ARCH)_$(PLATFORM).a
	$(CXX) $(LINKFLAGS) $(OBJECTS) libdcjit_$(ARCH)_$(PLATFORM).a -o dc

clean:
	rm *.o *.a

