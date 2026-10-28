@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /utf-8 /MTd /Z7 /I Source Tests/SimpleTimeTests.cpp Source/GamePad.cpp /Foobj/ /Feobj/SimpleTimeTests.exe /link /INCREMENTAL:NO /DEBUG:NONE
if errorlevel 1 exit /b 1
obj\SimpleTimeTests.exe --profile-loading
exit /b %errorlevel%
