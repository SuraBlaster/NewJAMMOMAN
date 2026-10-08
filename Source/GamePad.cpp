#include <windows.h>
#include <math.h>
#include <Xinput.h>
#include "GamePad.h"
#include <algorithm>

// 更新
void GamePad::Update()
{
	if (vibrating && std::chrono::steady_clock::now() >= vibration_end) StopVibration();
	axis_lx = axis_ly = 0.0f;
	axis_rx = axis_ry = 0.0f;
	trigger_l = trigger_r = 0.0f;

	GamePadButton newButtonState = 0;

	// ボタン情報取得
	XINPUT_STATE xinputState;
	if (XInputGetState(slot, &xinputState) == ERROR_SUCCESS)
	{
		//XINPUT_CAPABILITIES caps;
		//XInputGetCapabilities(m_slot, XINPUT_FLAG_GAMEPAD, &caps);
		XINPUT_GAMEPAD& pad = xinputState.Gamepad;

		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP)					newButtonState |= BTN_UP;
		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)				newButtonState |= BTN_RIGHT;
		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)				newButtonState |= BTN_DOWN;
		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)				newButtonState |= BTN_LEFT;
		if (pad.wButtons & XINPUT_GAMEPAD_A)						newButtonState |= BTN_A;
		if (pad.wButtons & XINPUT_GAMEPAD_B)						newButtonState |= BTN_B;
		if (pad.wButtons & XINPUT_GAMEPAD_X)						newButtonState |= BTN_X;
		if (pad.wButtons & XINPUT_GAMEPAD_Y)						newButtonState |= BTN_Y;
		if (pad.wButtons & XINPUT_GAMEPAD_START)					newButtonState |= BTN_START;
		if (pad.wButtons & XINPUT_GAMEPAD_BACK)						newButtonState |= BTN_BACK;
		if (pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)				newButtonState |= BTN_LEFT_THUMB;
		if (pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)				newButtonState |= BTN_RIGHT_THUMB;
		if (pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)			newButtonState |= BTN_LEFT_SHOULDER;
		if (pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)			newButtonState |= BTN_RIGHT_SHOULDER;
		if (pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)	newButtonState |= BTN_LEFT_TRIGGER;
		if (pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)	newButtonState |= BTN_RIGHT_TRIGGER;

		if ((pad.sThumbLX <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) &&
			(pad.sThumbLY <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
		{
			pad.sThumbLX = 0;
			pad.sThumbLY = 0;
		}

		if ((pad.sThumbRX <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) &&
			(pad.sThumbRY <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE))
		{
			pad.sThumbRX = 0;
			pad.sThumbRY = 0;
		}

		trigger_l = static_cast<float>(pad.bLeftTrigger) / 255.0f;
		trigger_r = static_cast<float>(pad.bRightTrigger) / 255.0f;
		axis_lx = static_cast<float>(pad.sThumbLX) / static_cast<float>(0x8000);
		axis_ly = static_cast<float>(pad.sThumbLY) / static_cast<float>(0x8000);
		axis_rx = static_cast<float>(pad.sThumbRX) / static_cast<float>(0x8000);
		axis_ry = static_cast<float>(pad.sThumbRY) / static_cast<float>(0x8000);
	}

	{
		button_state[1] = button_state[0];	// スイッチ履歴
		button_state[0] = newButtonState;

		button_down = ~button_state[1] & newButtonState;	// 押した瞬間
		button_up = ~newButtonState & button_state[1];	// 離した瞬間
	}
}

void GamePad::Vibrate(float left, float right, float duration)
{
    StopVibration();
    if (duration <= 0.0f) return;
    XINPUT_VIBRATION value{};
    value.wLeftMotorSpeed = static_cast<WORD>((std::clamp)(left, 0.0f, 1.0f) * 65535.0f);
    value.wRightMotorSpeed = static_cast<WORD>((std::clamp)(right, 0.0f, 1.0f) * 65535.0f);
    if (XInputSetState(slot, &value) != ERROR_SUCCESS) return;
    vibrating = true;
    vibration_end = std::chrono::steady_clock::now()
        + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(duration));
}

void GamePad::StopVibration()
{
    if (!vibrating) return;
    XINPUT_VIBRATION value{};
    XInputSetState(slot, &value);
    vibrating = false;
}