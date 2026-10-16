#include "Input.h"

// 更新処理
void Input::Update()
{
    previous = current;

    for (int key = 0; key < 256; ++key)
    {

        current[key] = key > VK_XBUTTON2 && (::GetAsyncKeyState(key) & 0x8000) != 0;
    }
}
