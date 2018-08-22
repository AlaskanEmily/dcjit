rem Any copyright is dedicated to the Public Domain.
rem http://creativecommons.org/publicdomain/zero/1.0/

rem Builds the emscripten configuration of DC
@echo off
cd src
emmake make BACKEND=js EXT=.js SO=lib.js emscripten || echo FAILED
cd ..
