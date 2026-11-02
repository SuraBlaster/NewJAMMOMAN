#include <windows.h>
#include <shellapi.h>
#include <cstddef>
#include <exception>
#include <vector>
#include <string>
#include "Converter.h"
#include "GuiApplication.h"

constexpr int CONVERTER_EXIT_SUCCESS = 0;
constexpr int EXIT_INVALID_ARGUMENTS = 1;
constexpr int EXIT_CONVERSION_FAILURE = 2;

static std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
    {
        return {};
    }

    const int requiredLength =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                            nullptr, 0, nullptr, nullptr);

    if (requiredLength <= 0)
    {
        return {};
    }

    std::string result(static_cast<std::size_t>(requiredLength), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                        result.data(), requiredLength, nullptr, nullptr);
    return result;
}

// 引数付き起動は自動テストや一括処理用として従来のCLI変換を維持する
static int RunCommandLineConversion()
{
    int argumentCount = 0;
    LPWSTR* wideArguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);

    if (wideArguments == nullptr)
    {
        return EXIT_INVALID_ARGUMENTS;
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

    ConverterOptions options;
    std::string errorMessage;

    if (!ParseCommandLine(argumentCount, argumentPointers.data(), options, errorMessage))
    {
        return EXIT_INVALID_ARGUMENTS;
    }

    if (options.showHelp)
    {
        return CONVERTER_EXIT_SUCCESS;
    }

    try
    {
        const TiledStageConverter converter;
        converter.Convert(options);
        return CONVERTER_EXIT_SUCCESS;
    }
    catch (const std::exception&)
    {
        return EXIT_CONVERSION_FAILURE;
    }
}

// ダブルクリック時はGUI、引数付き起動時はCLIとして動作する
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand)
{
    if (commandLine != nullptr && commandLine[0] != L'\0')
    {
        return RunCommandLineConversion();
    }

    return RunGuiApplication(instance, showCommand);
}
