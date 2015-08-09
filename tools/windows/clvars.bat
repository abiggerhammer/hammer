REM Don't call me directly
REM Exports CLFLAGS

REM Start with the most strict warning level
set WARNINGS=-W4 -Wall -WX

REM c4457 (declaration shadowing function parameter)
REM FIXME(windows) TODO(uucidl): remove occurence of c4457 and reactivate
REM FIXME(windows) TODO(uucidl): remove occurence of c4456 and reactivate
REM see -Wshadow
set WARNINGS=%WARNINGS% -wd4457 -wd4456

REM c4701 (potentially unitialized local variable)
REM FIXME(windows) TODO(uucidl): remove occurence of c4701 if possible
set WARNINGS=%WARNINGS% -wd4701

REM We disable implicit casting warnings (c4244), as they occur too often here.
REM Its gcc/clang counterpart is Wconversion which does not seem to
REM be enabled by default.
REM See: [[https://gcc.gnu.org/wiki/NewWconversion#Frequently_Asked_Questions]]
REM
REM Likewise for c4242 (conversion with potential loss of data) and c4267
REM (conversion away from size_t to a smaller type) and c4245 (conversion
REM from int to size_t signed/unsigned mismatch)
set WARNINGS=%WARNINGS% -wd4242 -wd4244 -wd4245 -wd4267

REM c4100 (unreferenced formal parameter) is equivalent to -Wno-unused-parameter
set WARNINGS=%WARNINGS% -wd4100

REM c4200 (zero-sized array) is a C idiom supported by C99
set WARNINGS=%WARNINGS% -wd4200

REM c4204 (non-constant aggregate initializers) ressembles C99 support
set WARNINGS=%WARNINGS% -wd4204

REM c4201 (anonymous unions) ressembles C11 support.
REM see -std=gnu99 vs -std=c99
set WARNINGS=%WARNINGS% -wd4201

REM c4820 (warnings about padding) and c4324 (intentional padding) are
REM not useful
set WARNINGS=%WARNINGS% -wd4820 -wd4324

REM c4710 (inlining could not be performed) is not useful
set WARNINGS=%WARNINGS% -wd4710

REM c4255 ( () vs (void) ambiguity) is not useful
set WARNINGS=%WARNINGS% -wd4255

REM c4127 (conditional expression is constant) is not useful
set WARNINGS=%WARNINGS% -wd4127

REM c4668 (an undefined symbol in a preprocessor directive) is not useful
set WARNINGS=%WARNINGS% -wd4668

REM we use sprintf so this should be enabled
set DEFINES=-D_CRT_SECURE_NO_WARNINGS

set CLFLAGS=-Od -Z7 %DEFINES% %WARNINGS% -Debug
