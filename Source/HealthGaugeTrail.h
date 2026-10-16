#pragma once
#include <algorithm>

struct HealthGaugeTrail
{
    float current = 1.0f;
    float trailing = 1.0f;
    float delay = 0.0f;
    static constexpr float HOLD_DURATION = 0.3f;
    static constexpr float DRAIN_SPEED = 0.65f;

    void Reset(float ratio)
    {
        current = trailing = std::clamp(ratio, 0.0f, 1.0f);
        delay = 0.0f;
    }

    // 更新処理
    void Update(float ratio, float elapsedTime)
    {
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        float remaining = (std::max)(0.0f, elapsedTime);

        if (ratio > current)
        {
            Reset(ratio);
            return;
        }

        if (ratio < current)
        {
            current = ratio;
            delay = HOLD_DURATION;
            return;
        }

        const float held = (std::min)(delay, remaining);
        delay -= held;
        remaining -= held;
        trailing = (std::max)(current, trailing - DRAIN_SPEED * remaining);
    }
};
