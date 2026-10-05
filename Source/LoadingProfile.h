#pragma once
#include <windows.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

// Opt-in, main-thread instrumentation. Nested totals include their children.
namespace LoadingProfile
{
    using Clock = std::chrono::steady_clock;
    inline bool Enabled() { static const bool value = wcsstr(GetCommandLineW(), L"--profile-loading") != nullptr; return value; }
    inline bool Automatic() { return wcsstr(GetCommandLineW(), L"--profile-loading-auto") != nullptr; }
    struct Entry { std::string name; double ms; };
    inline std::vector<Entry> entries;
    inline bool gameReady = false;
    inline bool firstGameFrame = false;
    inline Clock::time_point loadingStart, waitStart;
    inline double Milliseconds(Clock::time_point start) { return std::chrono::duration<double, std::milli>(Clock::now() - start).count(); }
    inline void Record(const std::string& name, double ms) { if (Enabled()) entries.push_back({name, ms}); }
    class Scope
    {
        std::string name;
        Clock::time_point start = Clock::now(), last = start;
    public:
        explicit Scope(std::string label) : name(std::move(label)) {}
        void Step(const char* label) { auto now = Clock::now(); Record(name + "/" + label, std::chrono::duration<double, std::milli>(now - last).count()); last = Clock::now(); }
        ~Scope() { Record(name + "/total", Milliseconds(start)); }
    };
    inline void Flush()
    {
        if (!Enabled() || entries.empty()) return;
        static bool first = true;
        std::ofstream out("loading-profile.log", std::ios::app);
        if (!out) return;
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
        for (const auto& e : entries) out << std::fixed << std::setprecision(3) << e.ms << " ms | " << e.name << '\n';
        entries.clear();
    }
}
