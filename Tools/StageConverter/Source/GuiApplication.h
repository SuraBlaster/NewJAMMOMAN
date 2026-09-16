#pragma once

#include <Windows.h>

namespace stage_converter
{
    // Win32ウィンドウ、DirectX 11、ImGuiを初期化してGUIを実行する。
    int RunGuiApplication(HINSTANCE instance, int showCommand);
}
