@echo off
rem $Id: mfmacro.bat,v 1.3 2001/09/14 03:16:13 cwolf Exp $
rem Creates and installs VC macro for exporting makefiles from 
rem the command line.
rem
rem To invoke the macro for exporting makefiles for the libraries: 
rem msdev -ex ExportMakefile
rem
rem To invloke the macro for exporting makfiles for the samples:
rem msdev -ex ExportExampleMakefiles
rem
if .%SRCROOT%==. goto notset
set macrofile="%msdevdir%\Macros\oggvorbis.dsm"
if exist %macrofile% goto done
echo 'This macro is for exporting makefiles for all projects > %macrofile%
echo 'from the command line. >> %macrofile%
echo 'To invoke: msdev -ex ExportMakefile
echo Sub ExportMakefile >> %macrofile%
echo   Application.Visible = False >> %macrofile%
echo   Documents.Open "%SRCROOT%\win32sdk\all.dsw",,True >> %macrofile%
echo   set ActiveProject = Projects("all") >> %macrofile%
echo   Application.ExecuteCommand "BuildProjectExport" >> %macrofile%
echo   Documents.SaveAll True >> %macrofile%
echo   Application.Quit >> %macrofile%
echo end Sub >> %macrofile%
echo ' >> %macrofile%
echo 'This macro is for exporting makefiles for example projects >> %macrofile%
echo 'from the command line. >> %macrofile%
echo 'To invoke: msdev -ex ExportMakefile
echo Sub ExportExampleMakefiles >> %macrofile%
echo   Application.Visible = False >> %macrofile%
echo   Documents.Open "%SRCROOT%\win32sdk\sdk\build\examples.dsw",,True >> %macrofile%
echo   set ActiveProject = Projects("examples") >> %macrofile%
echo   Application.ExecuteCommand "BuildProjectExport" >> %macrofile%
echo   Documents.SaveAll True >> %macrofile%
echo   Application.Quit >> %macrofile%
echo end Sub >> %macrofile%
set macrofile=
exit
:notset
echo Error SRCROOT not set
:done
