@echo off

set isMake=0
set isBuild=0

if "%1" equ "make"  set isMake=1 &                 goto MAKING
if "%1" equ "build" set isBuild=1 &                goto MAKING
if "%1" equ "full"  set isMake=1 & set isBuild=1 & goto MAKING
goto HINT

:MAKING
if %isMake%==0 goto BUILDING
echo Making GUI version...
cd ../..
rmdir generated\GUI /s /q
cd scripts/vs_win
@echo on
cmake ../../VS/CMakeLists.txt -B../../generated/GUI  -G "Visual Studio 17 2022" -A Win32 -DCMAKE_BUILD_TYPE=Release -DBUILD_USE_STATIC_RUNTIME=ON
@echo off

:BUILDING
if %isBuild%==0 goto EXIT
@echo on
MSBuild.exe ../../generated/GUI/RES-Network-GUI.sln -clp:ErrorsOnly;WarningsOnly -nologo /m
@echo off
goto EXIT

:HINT
echo.
echo Using assembly.bat:
echo                    assembly.bat [make^|build^|full]
echo.
goto EXIT

:EXIT
