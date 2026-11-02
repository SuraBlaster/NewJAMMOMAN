#include <windows.h>
#include <Xinput.h>
#include <cassert>
#include <cstdio>
#include "GamePad.h"
#include "LoadingProfile.h"
#include "TimeUtils.h"

static bool controllerConnected = true;
static XINPUT_VIBRATION lastVibration{};
static int vibrationCalls = 0;

// 実機を振動させずに入力状態を返す
DWORD WINAPI XInputGetState(DWORD, XINPUT_STATE* state) noexcept
{
    *state = {};
    return controllerConnected ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}

// 振動の指示を記録する
DWORD WINAPI XInputSetState(DWORD, XINPUT_VIBRATION* vibration) noexcept
{
    ++vibrationCalls;
    lastVibration = *vibration;
    return controllerConnected ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}

int main()
{
    const double startMilliseconds = GetTimeMilliseconds();
    Sleep(20);
    assert(GetTimeMilliseconds() - startMilliseconds >= 10.0);
    assert(LoadingProfile::Milliseconds(startMilliseconds) >= 10.0);
    assert(LoadingProfile::Enabled());
    LoadingProfile::entries.clear();

    {
        LoadingProfile::Scope profile("test");
        Sleep(10);
        profile.Step("step");
    }

    assert(LoadingProfile::entries.size() == 2);
    assert(LoadingProfile::entries[0].name == "test/step");
    assert(LoadingProfile::entries[0].ms >= 5.0);
    assert(LoadingProfile::entries[1].name == "test/total");
    assert(LoadingProfile::entries[1].ms >= LoadingProfile::entries[0].ms);

    GamePad pad;
    pad.Vibrate(0.8f, 0.4f, 0.04f);
    assert(lastVibration.wLeftMotorSpeed > lastVibration.wRightMotorSpeed);
    assert(lastVibration.wRightMotorSpeed > 0);
    const int startCalls = vibrationCalls;
    pad.Update();
    assert(vibrationCalls == startCalls);
    Sleep(60);
    pad.Update();
    assert(vibrationCalls == startCalls + 1);
    assert(lastVibration.wLeftMotorSpeed == 0 && lastVibration.wRightMotorSpeed == 0);
    pad.Update();
    assert(vibrationCalls == startCalls + 1);

    pad.Vibrate(1.0f, 1.0f, 1.0f);
    pad.Vibrate(1.0f, 1.0f, 0.0f);
    assert(lastVibration.wLeftMotorSpeed == 0 && lastVibration.wRightMotorSpeed == 0);

    controllerConnected = false;
    pad.Vibrate(1.0f, 1.0f, 0.01f);
    const int disconnectedCalls = vibrationCalls;
    Sleep(20);
    pad.Update();
    assert(vibrationCalls == disconnectedCalls);
    std::puts("PASS: millisecond timing, loading profile, vibration expiry, cancellation and disconnect.");
}
