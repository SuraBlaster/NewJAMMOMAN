#pragma once
#include <algorithm>
#include <cstddef>

template <class Keys>
bool FindAnimationKeyframes(const Keys& keys, float time, size_t& left, size_t& right, float& rate)
{
    if (keys.empty())
    {
        return false;
    }

    if (keys.size() == 1)
    {
        left = right = 0;
        rate = 0;
        return true;
    }

    if (time < keys.front().seconds || time > keys.back().seconds)
    {
        return false;
    }

    size_t lo = keys.size();

    if (keys.size() <= 64)
    {
        for (size_t i = 1; i < keys.size(); i++)
        {
            if (time < keys[i].seconds)
            {
                lo = i;
                break;
            }
        }
    }
    else
    {
        lo = 0;
        size_t hi = keys.size();

        while (lo < hi)
        {
            const size_t mid = lo + (hi - lo) / 2;

            if (time < keys[mid].seconds)
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }
    }

    right = lo == keys.size() ? keys.size() - 1 : lo;
    left = right - 1;
    const float duration = keys[right].seconds - keys[left].seconds;
    rate = duration > 0 ? (time - keys[left].seconds) / duration : 1.0f;
    return true;
}
