#include "InputManager.h"
#include <cmath>

void InputManager::Initialize(HWND hWnd)
{
    gamePad.StopVibration();
    keyboard = Input{};
    mouse = Mouse(hWnd);
    buttons = buttonDown = buttonUp = 0;
    axisLX = axisLY = axisRX = axisRY = 0.0f;
}

void InputManager::Update()
{
    keyboard.Update();
    mouse.Update();
    gamePad.Update();

    const GamePadButton padButtons = gamePad.GetButton();
    InputButton next = padButtons & ~GamePad::BTN_LEFT_TRIGGER;
    if (padButtons & GamePad::BTN_LEFT_TRIGGER) next |= BTN_B;
    const auto map = [&](int key, InputButton button)
    {
        if (keyboard.IsKeyPressed(key)) next |= button;
    };
    map('V', BTN_Y);
    map(VK_UP, BTN_UP); map(VK_RIGHT, BTN_RIGHT);
    map(VK_DOWN, BTN_DOWN); map(VK_LEFT, BTN_LEFT);
    // Jump: A / Space. Dash: B / LT / either Shift / right mouse. Attack: X / left mouse.
    map(VK_SPACE, BTN_A); map(VK_SHIFT, BTN_B);
    map(VK_LSHIFT, BTN_B); map(VK_RSHIFT, BTN_B);
    if (mouse.GetButton() & Mouse::BTN_LEFT) next |= BTN_X;
    if (mouse.GetButton() & Mouse::BTN_RIGHT) next |= BTN_B;

    // Compute edges after combining devices: releasing one source must not
    // release an action that is still held by another source.
    buttonDown = next & ~buttons;
    buttonUp = buttons & ~next;
    buttons = next;

    axisLX = gamePad.GetAxisLX(); axisLY = gamePad.GetAxisLY();
    axisRX = gamePad.GetAxisRX(); axisRY = gamePad.GetAxisRY();
    float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;
    if (keyboard.IsKeyPressed('W')) ly = 1.0f;
    if (keyboard.IsKeyPressed('A')) lx = -1.0f;
    if (keyboard.IsKeyPressed('S')) ly = -1.0f;
    if (keyboard.IsKeyPressed('D')) lx = 1.0f;
    if (keyboard.IsKeyPressed('I')) ry = 1.0f;
    if (keyboard.IsKeyPressed('J')) rx = -1.0f;
    if (keyboard.IsKeyPressed('K')) ry = -1.0f;
    if (keyboard.IsKeyPressed('L')) rx = 1.0f;
    if (next & BTN_UP) ly = 1.0f;
    if (next & BTN_RIGHT) lx = 1.0f;
    if (next & BTN_DOWN) ly = -1.0f;
    if (next & BTN_LEFT) lx = -1.0f;
    const auto overrideAxis = [](float x, float y, float& axisX, float& axisY)
    {
        const float length = std::sqrt(x * x + y * y);
        if (length > 0.0f) { axisX = x / length; axisY = y / length; }
    };
    overrideAxis(lx, ly, axisLX, axisLY);
    overrideAxis(rx, ry, axisRX, axisRY);
}
