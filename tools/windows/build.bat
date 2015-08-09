@echo off
setlocal

REM This script must be run after vcvarsall.bat has been run,
REM so that cl.exe is in your path.
where cl.exe || goto vsmissing_err

REM HEREPATH is <drive_letter>:<script_directory>
set HEREPATH=%~d0%~p0

REM Set up SRC, BUILD and CLFLAGS
call %HEREPATH%\env.bat
call %HEREPATH%\clvars.bat

echo SRC=%SRC%, BUILD=%BUILD%
echo Building with flags: %CLFLAGS%

pushd %SRC%
mkdir %BUILD%\obj
del /Q %BUILD%\obj\

cl.exe -nologo -FC -EHsc -Z7 -Oi -GR- -Gm- %CLFLAGS% -c ^
       @%HEREPATH%\hammer_lib_src_list ^
       -Fo%BUILD%\obj\
if %errorlevel% neq 0 goto err

lib.exe %BUILD%\obj\*.obj -OUT:%BUILD%\hammer.lib
echo STATIC_LIBRARY %BUILD%\hammer.lib
if %errorlevel% neq 0 goto err
popd

REM TODO(uucidl): how to build and run the tests? They are written with glib.h
REM which might be a challenge on windows. On the other hand the API of glib.h
REM does not seem too hard to reimplement.

echo SUCCESS: Successfully built
endlocal
exit /b 0

:vsmissing_err
echo ERROR: CL.EXE missing. Have you run vcvarsall.bat?
exit /b 1

:err
endlocal
echo ERROR: Failed to build
exit /b %errorlevel%
