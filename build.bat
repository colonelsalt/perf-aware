@echo off
IF NOT EXIST build mkdir build
pushd build

set IncludePath="..\computer_enhance\perfaware\sim86"
set LibPath="..\computer_enhance\perfaware\sim86\shared"
set Defines="/DPROFILER=1"

call cl -nologo -Zi -FC /FC /Zi -FC /I%IncludePath% /I%IncludePath%/shared ..\src\8086-simulator\main.cpp /link /LIBPATH:%LibPath% sim86_shared_debug.lib
call cl -nologo -Zi -FC /FC /Zi -FC ..\src\haver_input.cpp
call cl -nologo -Zi -FC /FC /Zi -FC ..\src\haver_solver.cpp %Defines%
call cl -nologo -Zi -FC /FC /Zi -FC ..\src\rep_test_main.cpp

popd