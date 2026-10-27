#pragma once

#include <windows.h>

class HighResolutionTimer
{
  public:

    // コンストラクタ
    HighResolutionTimer() : deltaTime(-1.0), pausedTime(0), stopped(false)
    {
        LONGLONG countsPerSec;
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
        secondsPerCount = 1.0 / static_cast<double>(countsPerSec);

        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&thisTime));
        baseTime = thisTime;
        lastTime = thisTime;
    }

    float TimeStamp() const
    {

        //

        if (stopped)
        {
            return static_cast<float>(((stopTime - pausedTime) - baseTime) * secondsPerCount);
        }

        //

        //

        else
        {
            return static_cast<float>(((thisTime - pausedTime) - baseTime) * secondsPerCount);
        }
    }

    float TimeInterval() const
    {
        return static_cast<float>(deltaTime);
    }

    void Reset()
    {
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&thisTime));
        baseTime = thisTime;
        lastTime = thisTime;

        stopTime = 0;
        stopped = false;
    }

    void Start()
    {
        LONGLONG startTime;
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

        //

        if (stopped)
        {
            pausedTime += (startTime - stopTime);
            lastTime = startTime;
            stopTime = 0;
            stopped = false;
        }
    }

    void Stop()
    {
        if (!stopped)
        {
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&stopTime));
            stopped = true;
        }
    }

    void Tick()
    {
        if (stopped)
        {
            deltaTime = 0.0;
            return;
        }

        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&thisTime));

        deltaTime = (thisTime - lastTime) * secondsPerCount;

        lastTime = thisTime;

        if (deltaTime < 0.0)
        {
            deltaTime = 0.0;
        }
    }

  private:

    double secondsPerCount;
    double deltaTime;

    LONGLONG baseTime;
    LONGLONG pausedTime;
    LONGLONG stopTime;
    LONGLONG lastTime;
    LONGLONG thisTime;

    bool stopped;
};
