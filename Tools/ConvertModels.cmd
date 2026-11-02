@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist obj mkdir obj
cl /nologo /EHsc /std:c++17 /utf-8 /MTd /Zi /D_DEBUG /I Source /I External/DirectXTex /I External/cereal/include /I External/tinygltf Tools/ConvertModels.cpp Source/Model.cpp Source/GLTFImporter.cpp Source/GpuResourceUtils.cpp External/DirectXTex/Bin/Desktop_2022/x64/Debug/DirectXTex.lib /Foobj/ /Feobj/ConvertModels.exe /Fdobj/ConvertModels.pdb /link d3d11.lib dxgi.lib windowscodecs.lib ole32.lib user32.lib
if errorlevel 1 exit /b 1
obj\ConvertModels.exe