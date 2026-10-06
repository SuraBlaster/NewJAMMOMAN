#pragma once

class HitStopManager
{
public:
    static HitStopManager& Instance()
    {
        static HitStopManager instance;
        return instance;
    }

    // ヒットストップの開始
    void Request(float duration, float scale = 0.05f)
    {
        if (duration > timer)
        {
            timer = duration;
            slowScale = scale;
        }
    }

    void Update(float rawDeltaTime)
    {
        if (timer > 0.0f)
        {
            timer -= rawDeltaTime;
            if (timer <= 0.0f)
            {
                timer = 0.0f;
                slowScale = 1.0f;
            }
        }
    }

    // 現在のタイムスケールを取得（通常時は 1.0f）
    float GetTimeScale() const
    {
        return (timer > 0.0f) ? slowScale : 1.0f;
    }

private:
    HitStopManager() = default;
    float timer = 0.0f;
    float slowScale = 1.0f;
};
