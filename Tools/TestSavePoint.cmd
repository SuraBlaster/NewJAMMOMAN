@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /utf-8 /EHsc /std:c++17 /I Source /I Tools/StageConverter/Source /I External/tinygltf Tests/SavePointTests.cpp Tools/StageConverter/Source/Converter.cpp Source/StageLoader.cpp Source/TileTypeManager.cpp /Foobj/ /Feobj/SavePointTests.exe /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
obj\SavePointTests.exe
exit /b %errorlevel%
