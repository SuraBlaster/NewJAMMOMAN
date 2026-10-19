@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /I Source /I External/tinygltf Tests/BossGateTests.cpp Source/StageLoader.cpp Source/TileTypeManager.cpp /Foobj/ /Feobj/BossGateTests.exe /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
obj\BossGateTests.exe
exit /b %errorlevel%
