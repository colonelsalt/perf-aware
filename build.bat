@echo off
IF NOT EXIST build mkdir build
pushd build

set IncludePath="..\computer_enhance\perfaware\sim86"
set LibPath="..\computer_enhance\perfaware\sim86\shared"

call cl -nologo -Zi -FC /FC /Zi -FC /I%IncludePath% /I%IncludePath%/shared ..\src\main.cpp /link /LIBPATH:%LibPath% sim86_shared_debug.lib

popd