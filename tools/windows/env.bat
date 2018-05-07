REM Don't call me directly.
REM
REM Expects HEREPATH (this directory)
REM Exports SRC (hammer's src directory)
REM Exports BUILD (hammer's build directory)

set TOP=%HEREPATH%..\..

REM Get canonical path for TOP
pushd .
cd %TOP%
set TOP=%CD%
popd

set SRC=%TOP%\src
set BUILD=%TOP%\build
