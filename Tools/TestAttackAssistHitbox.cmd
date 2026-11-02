@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist obj mkdir obj
cl /nologo /EHsc /std:c++17 /MTd /Z7 /I Source Tests/AttackAssistHitboxTests.cpp /Foobj/ /Feobj/AttackAssistHitboxTests.exe /link /INCREMENTAL:NO /DEBUG:NONE
if errorlevel 1 exit /b 1
obj\AttackAssistHitboxTests.exe
exit /b %errorlevel%
