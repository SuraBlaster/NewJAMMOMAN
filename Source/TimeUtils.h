#pragma once
#include <windows.h>

// 現在時刻をミリ秒単位の数値で取得する
inline double GetTimeMilliseconds()
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
}
