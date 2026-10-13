@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /MTd /Z7 /I Source Tests/PlayerTrailTests.cpp Tests/PlayerTrailTestSupport.cpp Source/PlayerTrail.cpp /Foobj/ /Feobj/PlayerTrailTests.exe /link /INCREMENTAL:NO /DEBUG:NONE
if errorlevel 1 exit /b 1
obj\PlayerTrailTests.exe
exit /b %errorlevel%
