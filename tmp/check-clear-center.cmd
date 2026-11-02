@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl.exe /Zs /EHsc /std:c++17 /utf-8 /D_DEBUG /D_WINDOWS /I Source /I External/imgui-docking /I External/ImGuizmo /I External/cereal/include /I External/tinygltf /I External/DirectXTex /I External/Effekseer170e/include/Effekseer /I External/Effekseer170e/include/EffekseerRendererDX11 Source/PlayerStates.cpp
