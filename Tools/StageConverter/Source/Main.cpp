#include "GuiApplication.h"
#include "Converter.h"

#include <Windows.h>
#include <shellapi.h>

#include <exception>
#include <string>
#include <vector>

namespace
{
    constexpr int kExitSuccess = 0;
    constexpr int kExitInvalidArguments = 1;
    constexpr int kExitConversionFailure = 2;

    std::string WideToUtf8(const std::wstring& text)
    {
        if (text.empty())
        {
            return {};
        }

        const int requiredLength = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
        if (requiredLength <= 0)
        {
            return {};
        }

        std::string result(static_cast<std::size_t>(requiredLength), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            requiredLength,
            nullptr,
            nullptr);
        return result;
    }

    // 引数付き起動は自動テストや一括処理用として従来のCLI変換を維持する。
    int RunCommandLineConversion()
    {
        int argumentCount = 0;
        LPWSTR* wideArguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
        if (wideArguments == nullptr)
        {
            return kExitInvalidArguments;
        }

        std::vector<std::string> utf8Arguments;
        utf8Arguments.reserve(static_cast<std::size_t>(argumentCount));
        for (int index = 0; index < argumentCount; ++index)
        {
            utf8Arguments.push_back(WideToUtf8(wideArguments[index]));
        }
        LocalFree(wideArguments);

        std::vector<char*> argumentPointers;
        argumentPointers.reserve(utf8Arguments.size());
        for (std::string& argument : utf8Arguments)
        {
            argumentPointers.push_back(argument.data());
        }

        stage_converter::ConverterOptions options;
        std::string errorMessage;
        if (!stage_converter::ParseCommandLine(
            argumentCount,
            argumentPointers.data(),
            options,
            errorMessage))
        {
            return kExitInvalidArguments;
        }
        if (options.showHelp)
        {
            return kExitSuccess;
        }

        try
        {
            const stage_converter::TiledStageConverter converter;
            converter.Convert(options);
            return kExitSuccess;
        }
        catch (const std::exception&)
        {
            return kExitConversionFailure;
        }
    }
}

// ダブルクリック時はGUI、引数付き起動時はCLIとして動作する。
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand)
{
    if (commandLine != nullptr && commandLine[0] != L'\0')
    {
        return RunCommandLineConversion();
    }
    return stage_converter::RunGuiApplication(instance, showCommand);
}
