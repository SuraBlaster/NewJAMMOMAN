@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /MTd /Z7 /I Source Tests/CharacterDamageTests.cpp Tests/CharacterDamageTestSupport.cpp Source/Character.cpp /Foobj/ /Feobj/CharacterDamageTests.exe /link /INCREMENTAL:NO /DEBUG:NONE
if errorlevel 1 exit /b 1
obj\CharacterDamageTests.exe
exit /b %errorlevel%
