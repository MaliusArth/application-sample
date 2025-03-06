@echo off
setlocal

set build_mode=Release
if not "%~1"=="" set "build_mode=%~1"

@REM cmake --build .\build --config %build_mode% --target clean

ctime -begin build_timings.ctm
cmake --build .\build --config %build_mode%
ctime -end   build_timings.ctm

ctime -begin run_timings.ctm
.\build\%build_mode%\pipeline_cli.exe data.json > result.txt
ctime -end   run_timings.ctm