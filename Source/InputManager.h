#pragma once
#include "GamePad.h"
#include "Input.h"
#include "Mouse.h"

using InputButton = unsigned int;

// Owns devices and combines their sampled state into the game's input mapping.
class InputManager
{
public:
	static constexpr InputButton BTN_UP				    = (1 <<  0);
	static constexpr InputButton BTN_RIGHT			    = (1 <<  1);
	static constexpr InputButton BTN_DOWN				= (1 <<  2);
	static constexpr InputButton BTN_LEFT				= (1 <<  3);
	static constexpr InputButton BTN_A				    = (1 <<  4);
	static constexpr InputButton BTN_B				    = (1 <<  5);
	static constexpr InputButton BTN_X				    = (1 <<  6);
	static constexpr InputButton BTN_Y				    = (1 <<  7);
	static constexpr InputButton BTN_START			    = (1 <<  8);
	static constexpr InputButton BTN_BACK				= (1 <<  9);
	static constexpr InputButton BTN_LEFT_THUMB		    = (1 << 10);
	static constexpr InputButton BTN_RIGHT_THUMB		= (1 << 11);
	static constexpr InputButton BTN_LEFT_SHOULDER	    = (1 << 12);
	static constexpr InputButton BTN_RIGHT_SHOULDER	    = (1 << 13);
	static constexpr InputButton BTN_LEFT_TRIGGER		= (1 << 14);
	static constexpr InputButton BTN_RIGHT_TRIGGER	    = (1 << 15);

    static InputManager& Instance()
    {
        static InputManager instance;
        return instance;
    }
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void Initialize(HWND hWnd);
    void Update();
    void SetWheel(int delta) { mouse.SetWheel(delta); }
    GamePad& GetGamePad() { return gamePad; }
    const GamePad& GetGamePad() const { return gamePad; }
    Input& GetInput() { return keyboard; }
    const Input& GetInput() const { return keyboard; }
    Mouse& GetMouse() { return mouse; }
    const Mouse& GetMouse() const { return mouse; }

    InputButton GetButton() const { return buttons; }
    InputButton GetButtonDown() const { return buttonDown; }
    InputButton GetButtonUp() const { return buttonUp; }
    float GetAxisLX() const { return axisLX; }
    float GetAxisLY() const { return axisLY; }
    float GetAxisRX() const { return axisRX; }
    float GetAxisRY() const { return axisRY; }
    float GetTriggerL() const { return gamePad.GetTriggerL(); }
    float GetTriggerR() const { return gamePad.GetTriggerR(); }

private:
    InputManager() = default;
    GamePad gamePad;
    Input keyboard;
    Mouse mouse{nullptr};
    InputButton buttons = 0;
    InputButton buttonDown = 0;
    InputButton buttonUp = 0;
    float axisLX = 0.0f, axisLY = 0.0f;
    float axisRX = 0.0f, axisRY = 0.0f;
};
