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
echo CLFLAGS=%CLFLAGS%

set HAMMERLIB=%BUILD%\hammer.lib

REM Now let's build some example programs

cl.exe -nologo %CLFLAGS% examples\base64.c %HAMMERLIB% -Fo%BUILD%\ -Fe%BUILD%\
if %errorlevel% neq 0 goto err
echo PROGRAM build\base64.exe
cl.exe -nologo %CLFLAGS% examples\base64_sem1.c %HAMMERLIB%  -Fo%BUILD%\ -Fe%BUILD%\
if %errorlevel% neq 0 goto err
echo PROGRAM build\base64_sem1.exe
cl.exe -nologo %CLFLAGS% examples\base64_sem2.c %HAMMERLIB%  -Fo%BUILD%\ -Fe%BUILD%\
if %errorlevel% neq 0 goto err
echo PROGRAM build\base64_sem2.exe

REM FIXME(windows) TODO(uucidl): dns.c only works on posix
REM cl.exe -nologo %CLFLAGS% examples\dns.c %HAMMERLIB% -Fo%BUILD%\ -Fe%BUILD%\
REM if %errorlevel% neq 0 goto err
REM echo PROGRAM build\dns.exe

REM FIXME(windows) TODO(uucidl): grammar.c needs to be fixed
cl.exe -nologo %CLFLAGS% examples\ties.c examples\grammar.c %HAMMERLIB% -Fo%BUILD%\ -Fe%BUILD%\
if %errorlevel% neq 0 goto err
echo PROGRAM build\ties.exe

echo SUCCESS: Successfully built
endlocal
exit /b 0

:vsmissing_err
echo ERROR: CL.EXE missing. Have you run vcvarsall.bat?
exit /b 1

:err
echo ERROR: Failed to build
endlocal
exit /b %errorlevel%
