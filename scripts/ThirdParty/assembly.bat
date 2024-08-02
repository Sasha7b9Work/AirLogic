@echo off

set isMake=0
set isBuild=0

if "%1" equ "make"  set isMake=1 &                 goto MAKING
if "%1" equ "build" set isBuild=1 &                goto MAKING
if "%1" equ "full"  set isMake=1 & set isBuild=1 & goto MAKING
goto HINT

:MAKING
if %isMake%==0 goto BUILDING
echo Making third party libraries ...
cd ../..
rmdir generated\ThirdParty /s /q
cd scripts/ThirdParty
@echo on
cmake ../../ThirdParty/wxWidgets/CMakeLists.txt -B../../generated/ThirdParty  -G "Visual Studio 17 2022" -A Win32 -DwxBUILD_SHARED=OFF -DwxBUILD_USE_STATIC_RUNTIME=ON -DwxBUILD_SAMPLES=ALL
@echo off

:BUILDING
if %isBuild%==0 goto EXIT
@echo on
MSBuild.exe ../../generated/ThirdParty/wxWidgets.sln -clp:ErrorsOnly;WarningsOnly -nologo /m
@echo off
goto EXIT

:HINT
echo.
echo Using assembly.bat:
echo                    assembly.bat [make^|build^|full]
echo.
goto EXIT

:EXIT
