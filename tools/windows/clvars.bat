REM Don't call me directly
REM Exports CLFLAGS

REM Start with the most strict warning level
set WARNINGS=-W4 -Wall -WX

REM We disable implicit casting warnings (c4244), as they occur too often here.
REM Its gcc/clang counterpart is Wconversion which does not seem to
REM be enabled by default.
REM See: [[https://gcc.gnu.org/wiki/NewWconversion#Frequently_Asked_Questions]]
set WARNINGS=%WARNINGS% -wd4244

REM c4100 (unreferenced formal parameter) is equivalent to -Wno-unused-parameter
set WARNINGS=%WARNINGS% -wd4100

REM c4200 (zero-sized array) is a C idiom supported by C99
set WARNINGS=%WARNINGS% -wd4200

REM c4204 (non-constant aggregate initializers) ressembles C99 support
set WARNINGS=%WARNINGS% -wd4204

REM c4820 (warnings about padding) is not useful
set WARNINGS=%WARNINGS% -wd4820

REM c4710 (inlining could not be performed) is not useful
set WARNINGS=%WARNINGS% -wd4710

REM c4255 ( () vs (void) ambiguity) is not useful
set WARNINGS=%WARNINGS% -wd4255

REM c4996 (deprecated functions)
set WARNINGS=%WARNINGS% -wd4996

REM we use sprintf
set DEFINES=-D_CRT_SECURE_NO_WARNINGS

set CLFLAGS=-Od -Z7 %DEFINES% %WARNINGS% -Debug
