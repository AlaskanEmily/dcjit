@echo off

rem Any copyright is dedicated to the Public Domain.
rem http://creativecommons.org/publicdomain/zero/1.0/

set "DCJITCLTEST=%TMP%\bat~%RANDOM%.tmp"
cl 2> %DCJITCLTEST% > nul
find "for x64" %DCJITCLTEST% > nul && set "DCJITARCH=amd64" || set "DCJITARCH=x86"
cd src
nmake.exe /nologo /f nmakefile DCJITARCH=%DCJITARCH% %* && copy /B dc.exe ..\dc.exe > nul || echo FAILED
cd ..
