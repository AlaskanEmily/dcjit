# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

# nmake makefile to build the Emscripten backend.

SYSFLAGS=-g -O2
LINKFLAGS=-s WASM=0 $(SYSFLAGS)
CCFLAGS=-Wall -Wextra -pedantic -Werror $(SYSFLAGS)
CFLAGS=$(CCFLAGS) -ansi -Wenum-compare -Wshadow
CXXFLAGS=$(CCFLAGS) -std=c++14 -fno-rtti -fno-exceptions

dc_core.bc: dc_core.c dc.h dc_backend.h
	$(CC) $(CFLAGS) -c dc_core.c -o dc_core.bc

# Emscripten components
dc_js.bc: dc_js.cpp dc_backend.h
	$(CXX) $(CXXFLAGS) -c dc_js.cpp -o dc_js.bc

OBJECTS=dc_core.bc dc_js.bc
dcjit.js: $(OBJECTS)
	$(CXX) $(LINKFLAGS) -shared $(OBJECTS) --pre-js dc_jit_js.js -o dcjit.js
