@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /MTd /Zi /D_DEBUG /I Source /I External/DirectXTex Tests/ModelManagerTests.cpp obj/x64/Debug/ModelManager.obj obj/x64/Debug/Model.obj obj/x64/Debug/GLTFImporter.obj obj/x64/Debug/GpuResourceUtils.obj External/DirectXTex/Bin/Desktop_2022/x64/Debug/DirectXTex.lib /Foobj/ModelManagerTests.obj /Feobj/ModelManagerTests.exe /Fdobj/ModelManagerTests.pdb /link d3d11.lib dxgi.lib windowscodecs.lib ole32.lib user32.lib
if errorlevel 1 exit /b 1
obj\ModelManagerTests.exe

