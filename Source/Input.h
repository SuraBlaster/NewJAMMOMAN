#pragma once
#include <Windows.h>
#include <array>

class Input
{
  public:

    // 更新処理
    void Update();

    bool IsKeyPressed(int key) const
    {
        return Valid(key) && current[key];
    }

    bool IsKeyDown(int key) const
    {
        return Valid(key) && current[key] && !previous[key];
    }

    bool IsKeyUp(int key) const
    {
        return Valid(key) && !current[key] && previous[key];
    }

  private:

    static bool Valid(int key)
    {
        return key >= 0 && key < 256;
    }

    std::array<bool, 256> current{};
    std::array<bool, 256> previous{};
};
