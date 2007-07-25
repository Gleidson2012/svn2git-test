@echo off
echo ---+++--- Building Vorbisenc (Dynamic) ---+++---

if .%SRCROOT%==. set SRCROOT=i:\xiph
           
set OLDPATH=%PATH%
set OLDINCLUDE=%INCLUDE%
set OLDLIB=%LIB%

call "c:\program files\microsoft visual studio\vc98\bin\vcvars32.bat"
echo Setting include/lib paths for Vorbis
set INCLUDE=%INCLUDE%;%SRCROOT%\ogg\include;c:\src\vorbis\include
set LIB=%LIB%;%SRCROOT%\ogg\win32\Dynamic_Release;%SRCROOT%\vorbis\win32\Dynamic_Release
echo Compiling...
msdev vorbisenc_dynamic.dsp /useenv /make "vorbisenc_dynamic - Win32 Release" /rebuild

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%
