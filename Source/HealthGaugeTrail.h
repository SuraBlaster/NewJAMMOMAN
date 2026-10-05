#pragma once
#include <algorithm>

// A delayed damage layer beneath the immediate health fill.
struct HealthGaugeTrail
{
    float current = 1.0f;
    float trailing = 1.0f;
    float delay = 0.0f;
    static constexpr float holdDuration = 0.3f;
    static constexpr float drainSpeed = 0.65f;

    void Reset(float ratio)
    {
        current = trailing = std::clamp(ratio, 0.0f, 1.0f);
        delay = 0.0f;
    }

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
            delay = holdDuration;
            return;
        }
        const float held = (std::min)(delay, remaining);
        delay -= held;
        remaining -= held;
        trailing = (std::max)(current, trailing - drainSpeed * remaining);
    }
};
