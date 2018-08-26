@echo off

rem Any copyright is dedicated to the Public Domain.
rem http://creativecommons.org/publicdomain/zero/1.0/

rem Builds the emscripten configuration of DC

cd src
emmake nmake /f emscripten.mak dcjit.js || echo FAILED
cd ..
