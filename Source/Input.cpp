#include "Input.h"

void Input::Update()
{
    previous = current;
    for (int key = 0; key < 256; ++key)
    {
        // Mouse buttons are sampled by Mouse.
        current[key] = key > VK_XBUTTON2 && (::GetAsyncKeyState(key) & 0x8000) != 0;
    }
}
