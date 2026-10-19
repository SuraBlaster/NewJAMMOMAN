#include "GuiApplication.h"

#include "Converter.h"

#include <Windows.h>
#include <d3d11.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wordParameter,
    LPARAM longParameter);

namespace stage_converter
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr wchar_t WINDOW_CLASS_NAME[] = L"StageConverterWindowClass";
        constexpr wchar_t WINDOW_TITLE[] = L"Stage Converter";
        constexpr int INITIAL_WINDOW_WIDTH = 820;
        constexpr int INITIAL_WINDOW_HEIGHT = 440;
        constexpr UINT SWAP_CHAIN_BUFFER_COUNT = 2;
        constexpr UINT PRESENT_SYNC_INTERVAL = 1;
        constexpr float CLEAR_COLOR[] = { 0.10f, 0.11f, 0.13f, 1.0f };
        constexpr float JAPANESE_FONT_SIZE = 18.0f;
        constexpr wchar_t JAPANESE_FONT_FILE_NAME[] = L"meiryo.ttc";
        constexpr std::size_t PATH_BUFFER_SIZE = 2048;
        constexpr std::size_t STAGE_NAME_BUFFER_SIZE = 256;
        constexpr double GUI_DEFAULT_PIXELS_PER_UNIT = 32.0;
        constexpr double PIXELS_PER_UNIT_STEP = 1.0;
        constexpr double PIXELS_PER_UNIT_FAST_STEP = 16.0;
        constexpr WPARAM SYSTEM_COMMAND_MASK = 0xFFF0U;
        constexpr int CONVERTER_EXIT_SUCCESS = 0;
        constexpr int EXIT_INITIALIZATION_FAILURE = 1;

        struct DirectXResources
        {
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            ComPtr<IDXGISwapChain> swapChain;
            ComPtr<ID3D11RenderTargetView> renderTargetView;
        };

        struct GuiState
        {
            std::array<char, PATH_BUFFER_SIZE> inputPath{};
            std::array<char, PATH_BUFFER_SIZE> outputPath{};
            std::array<char, STAGE_NAME_BUFFER_SIZE> stageName{};
            double pixelsPerUnit = GUI_DEFAULT_PIXELS_PER_UNIT;
            std::string statusMessage = u8"入力ファイルと出力先を指定してください。";
            bool statusIsError = false;
        };

        DirectXResources gDirectX;
        HWND gMainWindow = nullptr;

        std::wstring Utf8ToWide(const std::string& text)
        {
            if (text.empty())
            {
                return {};
            }

            const int requiredLength = MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                static_cast<int>(text.size()),
                nullptr,
                0);
            if (requiredLength <= 0)
            {
                throw std::runtime_error("UTF-8文字列をWindows文字列へ変換できません。");
            }

            std::wstring converted(static_cast<std::size_t>(requiredLength), L'\0');
            MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                static_cast<int>(text.size()),
                converted.data(),
                requiredLength);
            return converted;
        }

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
                throw std::runtime_error("Windows文字列をUTF-8へ変換できません。");
            }

            std::string converted(static_cast<std::size_t>(requiredLength), '\0');
            WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                text.data(),
                static_cast<int>(text.size()),
                converted.data(),
                requiredLength,
                nullptr,
                nullptr);
            return converted;
        }

        std::string HResultMessage(HRESULT result)
        {
            std::ostringstream message;
            message << "HRESULT=0x" << std::hex << std::uppercase
                << static_cast<unsigned long>(result);
            return message.str();
        }

        void ThrowIfFailed(HRESULT result, const std::string& context)
        {
            if (FAILED(result))
            {
                throw std::runtime_error(context + " " + HResultMessage(result));
            }
        }

        template<std::size_t BufferSize>
        void CopyToBuffer(const std::string& value, std::array<char, BufferSize>& buffer)
        {
            if (value.size() >= buffer.size())
            {
                throw std::runtime_error("選択したパスが入力欄の上限を超えています。");
            }

            buffer.fill('\0');
            std::copy(value.begin(), value.end(), buffer.begin());
        }

        std::string PathToUtf8(const std::filesystem::path& path)
        {
            return WideToUtf8(path.wstring());
        }

        std::filesystem::path Utf8ToPath(const char* text)
        {
            return std::filesystem::path(Utf8ToWide(text));
        }

        std::optional<std::filesystem::path> GetDialogResult(IFileDialog* dialog)
        {
            const HRESULT showResult = dialog->Show(gMainWindow);
            if (showResult == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                return std::nullopt;
            }
            ThrowIfFailed(showResult, "ファイル選択ダイアログを表示できません。");

            ComPtr<IShellItem> selectedItem;
            ThrowIfFailed(
                dialog->GetResult(&selectedItem),
                "選択したファイルを取得できません。");

            PWSTR selectedPath = nullptr;
            ThrowIfFailed(
                selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath),
                "選択したファイルのパスを取得できません。");

            const std::filesystem::path result(selectedPath);
            CoTaskMemFree(selectedPath);
            return result;
        }

        std::optional<std::filesystem::path> ShowOpenTiledDialog()
        {
            ComPtr<IFileOpenDialog> dialog;
            ThrowIfFailed(
                CoCreateInstance(
                    CLSID_FileOpenDialog,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&dialog)),
                "入力ファイル選択ダイアログを作成できません。");

            constexpr COMDLG_FILTERSPEC FILTERS[] = {
                { L"Tiled JSON map (*.tmj)", L"*.tmj" },
                { L"JSON file (*.json)", L"*.json" },
                { L"All files (*.*)", L"*.*" }
            };
            ThrowIfFailed(
                dialog->SetFileTypes(static_cast<UINT>(std::size(FILTERS)), FILTERS),
                "入力ファイルのフィルターを設定できません。");
            dialog->SetTitle(L"変換するTiledマップを選択");
            return GetDialogResult(dialog.Get());
        }

        std::optional<std::filesystem::path> ShowSaveStageDialog()
        {
            ComPtr<IFileSaveDialog> dialog;
            ThrowIfFailed(
                CoCreateInstance(
                    CLSID_FileSaveDialog,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&dialog)),
                "出力ファイル選択ダイアログを作成できません。");

            constexpr COMDLG_FILTERSPEC FILTERS[] = {
                { L"Stage JSON (*.stage.json)", L"*.stage.json" },
                { L"JSON file (*.json)", L"*.json" },
                { L"All files (*.*)", L"*.*" }
            };
            ThrowIfFailed(
                dialog->SetFileTypes(static_cast<UINT>(std::size(FILTERS)), FILTERS),
                "出力ファイルのフィルターを設定できません。");
            dialog->SetTitle(L"ゲーム用ステージJSONの保存先を選択");
            dialog->SetDefaultExtension(L"stage.json");
            dialog->SetFileName(L"ConvertedStage.stage.json");
            return GetDialogResult(dialog.Get());
        }

        void CreateRenderTarget()
        {
            ComPtr<ID3D11Texture2D> backBuffer;
            ThrowIfFailed(
                gDirectX.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)),
                "スワップチェーンのバックバッファを取得できません。");
            ThrowIfFailed(
                gDirectX.device->CreateRenderTargetView(
                    backBuffer.Get(),
                    nullptr,
                    &gDirectX.renderTargetView),
                "描画先を作成できません。");
        }

        void CreateDirectXDevice(HWND window)
        {
            DXGI_SWAP_CHAIN_DESC swapChainDescription{};
            swapChainDescription.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
            swapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDescription.OutputWindow = window;
            swapChainDescription.SampleDesc.Count = 1;
            swapChainDescription.Windowed = TRUE;
            swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            constexpr D3D_FEATURE_LEVEL REQUESTED_FEATURE_LEVELS[] = {
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_0
            };
            constexpr D3D_DRIVER_TYPE DRIVER_TYPES[] = {
                D3D_DRIVER_TYPE_HARDWARE,
                D3D_DRIVER_TYPE_WARP
            };

            HRESULT createResult = E_FAIL;
            D3D_FEATURE_LEVEL createdFeatureLevel{};
            for (const D3D_DRIVER_TYPE driverType : DRIVER_TYPES)
            {
                createResult = D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    driverType,
                    nullptr,
                    0,
                    REQUESTED_FEATURE_LEVELS,
                    static_cast<UINT>(std::size(REQUESTED_FEATURE_LEVELS)),
                    D3D11_SDK_VERSION,
                    &swapChainDescription,
                    &gDirectX.swapChain,
                    &gDirectX.device,
                    &createdFeatureLevel,
                    &gDirectX.context);
                if (SUCCEEDED(createResult))
                {
                    break;
                }
            }

            ThrowIfFailed(createResult, "DirectX 11を初期化できません。");
            CreateRenderTarget();
        }

        void LoadJapaneseFont()
        {
            wchar_t windowsDirectory[MAX_PATH]{};
            const UINT directoryLength = GetWindowsDirectoryW(
                windowsDirectory,
                static_cast<UINT>(std::size(windowsDirectory)));
            if (directoryLength == 0 || directoryLength >= std::size(windowsDirectory))
            {
                return;
            }

            const std::filesystem::path fontPath =
                std::filesystem::path(windowsDirectory) / L"Fonts" / JAPANESE_FONT_FILE_NAME;
            if (!std::filesystem::exists(fontPath))
            {
                return;
            }

            const std::string fontPathUtf8 = PathToUtf8(fontPath);
            ImGuiIO& inputOutput = ImGui::GetIO();
            inputOutput.Fonts->AddFontFromFileTTF(
                fontPathUtf8.c_str(),
                JAPANESE_FONT_SIZE,
                nullptr,
                inputOutput.Fonts->GetGlyphRangesJapanese());
        }

        void SetError(GuiState& state, const std::string& message)
        {
            state.statusMessage = message;
            state.statusIsError = true;
        }

        void SelectInputFile(GuiState& state)
        {
            try
            {
                const std::optional<std::filesystem::path> selectedPath = ShowOpenTiledDialog();
                if (!selectedPath.has_value())
                {
                    return;
                }

                CopyToBuffer(PathToUtf8(*selectedPath), state.inputPath);

                // 出力欄が空の場合だけ、入力ファイルと同じ場所へ候補名を設定する。
                if (state.outputPath.front() == '\0')
                {
                    std::filesystem::path suggestedOutput = *selectedPath;
                    suggestedOutput.replace_extension(L".stage.json");
                    if (selectedPath->filename() == L"LaboratoryCourse.tmj" &&
                        selectedPath->parent_path().filename() == L"Stage" &&
                        selectedPath->parent_path().parent_path().filename() == L"Data")
                    {
                        suggestedOutput = selectedPath->parent_path() / L"ConvertedStage.stage.json";
                    }
                    CopyToBuffer(PathToUtf8(suggestedOutput), state.outputPath);
                }

                state.statusMessage = u8"入力ファイルを選択しました。";
                state.statusIsError = false;
            }
            catch (const std::exception& error)
            {
                SetError(state, error.what());
            }
        }

        void SelectOutputFile(GuiState& state)
        {
            try
            {
                const std::optional<std::filesystem::path> selectedPath = ShowSaveStageDialog();
                if (!selectedPath.has_value())
                {
                    return;
                }

                CopyToBuffer(PathToUtf8(*selectedPath), state.outputPath);
                state.statusMessage = u8"出力先を選択しました。";
                state.statusIsError = false;
            }
            catch (const std::exception& error)
            {
                SetError(state, error.what());
            }
        }

        void ExecuteConversion(GuiState& state)
        {
            if (state.inputPath.front() == '\0')
            {
                SetError(state, u8"入力TMJファイルを指定してください。");
                return;
            }
            if (state.outputPath.front() == '\0')
            {
                SetError(state, u8"出力するstage.jsonのパスを指定してください。");
                return;
            }
            if (!std::isfinite(state.pixelsPerUnit) || state.pixelsPerUnit <= 0.0)
            {
                SetError(state, u8"Pixels Per Unitは0より大きい有限値にしてください。");
                return;
            }

            try
            {
                ConverterOptions options;
                options.inputPath = Utf8ToPath(state.inputPath.data());
                options.outputPath = Utf8ToPath(state.outputPath.data());
                options.pixelsPerUnit = state.pixelsPerUnit;
                if (state.stageName.front() != '\0')
                {
                    options.stageNameOverride = state.stageName.data();
                }

                const TiledStageConverter converter;
                const ConversionResult result = converter.Convert(options);

                std::ostringstream message;
                message
                    << u8"変換に成功しました。\n"
                    << u8"ステージ名: " << result.stageName << '\n'
                    << u8"変換した地形数: " << result.convertedObjectCount << '\n'
                    << u8"変換した敵数: " << result.convertedEnemyCount << '\n'
                    << u8"変換した落下復帰エリア数: " << result.convertedFallRespawnZoneCount << '\n'
                    << u8"変換したボス戦開始エリア数: " << result.convertedBossEncounterZoneCount << '\n'
                    << u8"変換したボス前通路カメラエリア数: " << result.convertedBossApproachCameraZoneCount << '\n'
                    << u8"変換したカメラ表示範囲数: " << result.convertedCameraBoundsCount << '\n'
                    << u8"スキップした配置数: " << result.skippedObjectCount;
                state.statusMessage = message.str();
                state.statusIsError = false;
            }
            catch (const std::exception& error)
            {
                SetError(state, std::string(u8"変換に失敗しました。\n") + error.what());
            }
        }

        void DrawGui(GuiState& state)
        {
            const ImGuiIO& inputOutput = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(inputOutput.DisplaySize);

            constexpr ImGuiWindowFlags WINDOW_FLAGS =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse;

            ImGui::Begin("StageConverterMainWindow", nullptr, WINDOW_FLAGS);
            ImGui::TextUnformatted(u8"Tiled ステージコンバーター");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextUnformatted(u8"入力TMJファイル");
            ImGui::SetNextItemWidth(-90.0f);
            ImGui::InputTextWithHint(
                "##InputPath",
                u8"変換する .tmj ファイル",
                state.inputPath.data(),
                state.inputPath.size());
            ImGui::SameLine();
            if (ImGui::Button(u8"参照##Input"))
            {
                SelectInputFile(state);
            }

            ImGui::Spacing();
            ImGui::TextUnformatted(u8"出力stage.json");
            ImGui::SetNextItemWidth(-90.0f);
            ImGui::InputTextWithHint(
                "##OutputPath",
                u8"保存する .stage.json ファイル",
                state.outputPath.data(),
                state.outputPath.size());
            ImGui::SameLine();
            if (ImGui::Button(u8"参照##Output"))
            {
                SelectOutputFile(state);
            }

            ImGui::Spacing();
            ImGui::TextUnformatted(u8"Pixels Per Unit");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::InputDouble(
                "##PixelsPerUnit",
                &state.pixelsPerUnit,
                PIXELS_PER_UNIT_STEP,
                PIXELS_PER_UNIT_FAST_STEP,
                "%.3f");
            ImGui::SameLine();
            ImGui::TextDisabled(u8"Tiledの何ピクセルをゲーム内の1単位にするか");

            ImGui::Spacing();
            ImGui::TextUnformatted(u8"ステージ名（任意）");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint(
                "##StageName",
                u8"空欄ならTiledのstageName、または入力ファイル名を使用",
                state.stageName.data(),
                state.stageName.size());

            ImGui::Spacing();
            if (ImGui::Button(u8"変換実行", ImVec2(150.0f, 38.0f)))
            {
                ExecuteConversion(state);
            }

            ImGui::Spacing();
            ImGui::Separator();
            const ImVec4 statusColor = state.statusIsError
                ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
                : ImVec4(0.45f, 0.90f, 0.55f, 1.0f);
            ImGui::TextColored(statusColor, "%s", state.statusMessage.c_str());
            ImGui::End();
        }

        LRESULT CALLBACK WindowProcedure(
            HWND window,
            UINT message,
            WPARAM wordParameter,
            LPARAM longParameter)
        {
            if (ImGui_ImplWin32_WndProcHandler(window, message, wordParameter, longParameter))
            {
                return TRUE;
            }

            switch (message)
            {
            case WM_SIZE:
                if (gDirectX.swapChain && wordParameter != SIZE_MINIMIZED)
                {
                    gDirectX.renderTargetView.Reset();
                    const UINT width = static_cast<UINT>(LOWORD(longParameter));
                    const UINT height = static_cast<UINT>(HIWORD(longParameter));
                    if (SUCCEEDED(gDirectX.swapChain->ResizeBuffers(
                        0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
                    {
                        try
                        {
                            CreateRenderTarget();
                        }
                        catch (const std::exception&)
                        {
                            PostQuitMessage(EXIT_INITIALIZATION_FAILURE);
                        }
                    }
                }
                return 0;

            case WM_SYSCOMMAND:
                if ((wordParameter & SYSTEM_COMMAND_MASK) == SC_KEYMENU)
                {
                    return 0;
                }
                break;

            case WM_DESTROY:
                PostQuitMessage(CONVERTER_EXIT_SUCCESS);
                return 0;
            }

            return DefWindowProcW(window, message, wordParameter, longParameter);
        }
    }

    int RunGuiApplication(HINSTANCE instance, int showCommand)
    {
        ImGui_ImplWin32_EnableDpiAwareness();

        const HRESULT comResult = CoInitializeEx(
            nullptr,
            COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(comResult))
        {
            MessageBoxW(
                nullptr,
                L"Windowsのファイル選択機能を初期化できません。",
                WINDOW_TITLE,
                MB_OK | MB_ICONERROR);
            return EXIT_INITIALIZATION_FAILURE;
        }

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_CLASSDC;
        windowClass.lpfnWndProc = WindowProcedure;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = WINDOW_CLASS_NAME;

        if (RegisterClassExW(&windowClass) == 0)
        {
            CoUninitialize();
            return EXIT_INITIALIZATION_FAILURE;
        }

        const HWND window = CreateWindowW(
            WINDOW_CLASS_NAME,
            WINDOW_TITLE,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            INITIAL_WINDOW_WIDTH,
            INITIAL_WINDOW_HEIGHT,
            nullptr,
            nullptr,
            instance,
            nullptr);
        if (window == nullptr)
        {
            UnregisterClassW(WINDOW_CLASS_NAME, instance);
            CoUninitialize();
            return EXIT_INITIALIZATION_FAILURE;
        }
        gMainWindow = window;

        try
        {
            CreateDirectXDevice(window);
        }
        catch (const std::exception& error)
        {
            const std::wstring wideMessage = Utf8ToWide(error.what());
            MessageBoxW(window, wideMessage.c_str(), WINDOW_TITLE, MB_OK | MB_ICONERROR);
            DestroyWindow(window);
            UnregisterClassW(WINDOW_CLASS_NAME, instance);
            CoUninitialize();
            return EXIT_INITIALIZATION_FAILURE;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        LoadJapaneseFont();
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX11_Init(gDirectX.device.Get(), gDirectX.context.Get());

        ShowWindow(window, showCommand);
        UpdateWindow(window);

        GuiState guiState;
        bool running = true;
        while (running)
        {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
                if (message.message == WM_QUIT)
                {
                    running = false;
                }
            }
            if (!running)
            {
                break;
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            DrawGui(guiState);
            ImGui::Render();

            gDirectX.context->OMSetRenderTargets(
                1,
                gDirectX.renderTargetView.GetAddressOf(),
                nullptr);
            gDirectX.context->ClearRenderTargetView(
                gDirectX.renderTargetView.Get(),
                CLEAR_COLOR);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            gDirectX.swapChain->Present(PRESENT_SYNC_INTERVAL, 0);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        gDirectX.renderTargetView.Reset();
        gDirectX.swapChain.Reset();
        gDirectX.context.Reset();
        gDirectX.device.Reset();

        DestroyWindow(window);
        gMainWindow = nullptr;
        UnregisterClassW(WINDOW_CLASS_NAME, instance);
        CoUninitialize();
        return CONVERTER_EXIT_SUCCESS;
    }
}
