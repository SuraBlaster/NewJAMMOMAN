#pragma once
#include <windows.h>
#include <utility>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <mutex>
#include "TimeUtils.h"

namespace LoadingProfile
{

    inline bool Enabled()
    {
        static const bool value = wcsstr(GetCommandLineW(), L"--profile-loading") != nullptr;
        return value;
    }

    inline bool Automatic()
    {
        return wcsstr(GetCommandLineW(), L"--profile-loading-auto") != nullptr;
    }

    struct Entry
    {
        std::string name;
        double ms;
    };

    inline std::vector<Entry> entries;
    inline std::mutex entriesMutex;
    inline bool gameReady = false;
    inline bool firstGameFrame = false;
    inline double loadingStart = 0.0;
    inline double waitStart = 0.0;

    inline double Milliseconds(double start)
    {
        return GetTimeMilliseconds() - start;
    }

    inline void Record(const std::string& name, double ms)
    {
        if (Enabled())
        {
            std::lock_guard<std::mutex> lock(entriesMutex);
            entries.push_back({name, ms});
        }
    }

    class Scope
    {
        std::string name;
        double start = GetTimeMilliseconds();
        double last = start;

    public:
        // コンストラクタ
        explicit Scope(std::string label) : name(std::move(label)) {}

        void Step(const char* label)
        {
            const double now = GetTimeMilliseconds();
            Record(name + "/" + label, now - last);
            last = GetTimeMilliseconds();
        }

        // デストラクタ
        ~Scope()
        {
            Record(name + "/total", Milliseconds(start));
        }
    };

    inline void Flush()
    {
        if (!Enabled())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(entriesMutex);

        if (entries.empty())
        {
            return;
        }

        static bool first = true;
        std::ofstream out("loading-profile.log", std::ios::app);

        if (!out)
        {
            return;
        }

        if (first)
        {
#ifdef _DEBUG
            out << "\nSESSION Debug";
#else
            out << "\nSESSION Release";
#endif
            out << " pid=" << GetCurrentProcessId() << " (milliseconds; nested totals overlap)\n";
            first = false;
        }

        for (const auto& e : entries)
        {
            out << std::fixed << std::setprecision(3) << e.ms << " ms | " << e.name << '\n';
        }

        entries.clear();
    }
}
