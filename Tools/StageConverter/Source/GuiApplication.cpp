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

        constexpr wchar_t kWindowClassName[] = L"StageConverterWindowClass";
        constexpr wchar_t kWindowTitle[] = L"Stage Converter";
        constexpr int kInitialWindowWidth = 820;
        constexpr int kInitialWindowHeight = 440;
        constexpr UINT kSwapChainBufferCount = 2;
        constexpr UINT kPresentSyncInterval = 1;
        constexpr float kClearColor[] = { 0.10f, 0.11f, 0.13f, 1.0f };
        constexpr float kJapaneseFontSize = 18.0f;
        constexpr wchar_t kJapaneseFontFileName[] = L"meiryo.ttc";
        constexpr std::size_t kPathBufferSize = 2048;
        constexpr std::size_t kStageNameBufferSize = 256;
        constexpr double kGuiDefaultPixelsPerUnit = 32.0;
        constexpr double kPixelsPerUnitStep = 1.0;
        constexpr double kPixelsPerUnitFastStep = 16.0;
        constexpr WPARAM kSystemCommandMask = 0xFFF0U;
        constexpr int kExitSuccess = 0;
        constexpr int kExitInitializationFailure = 1;

        struct DirectXResources
        {
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            ComPtr<IDXGISwapChain> swapChain;
            ComPtr<ID3D11RenderTargetView> renderTargetView;
        };

        struct GuiState
        {
            std::array<char, kPathBufferSize> inputPath{};
            std::array<char, kPathBufferSize> outputPath{};
            std::array<char, kStageNameBufferSize> stageName{};
            double pixelsPerUnit = kGuiDefaultPixelsPerUnit;
            std::string statusMessage = u8"入力ファイルと出力先を指定してください。";
            bool statusIsError = false;
        };

        DirectXResources g_directX;
        HWND g_mainWindow = nullptr;

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
            const HRESULT showResult = dialog->Show(g_mainWindow);
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

            constexpr COMDLG_FILTERSPEC filters[] = {
                { L"Tiled JSON map (*.tmj)", L"*.tmj" },
                { L"JSON file (*.json)", L"*.json" },
                { L"All files (*.*)", L"*.*" }
            };
            ThrowIfFailed(
                dialog->SetFileTypes(static_cast<UINT>(std::size(filters)), filters),
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

            constexpr COMDLG_FILTERSPEC filters[] = {
                { L"Stage JSON (*.stage.json)", L"*.stage.json" },
                { L"JSON file (*.json)", L"*.json" },
                { L"All files (*.*)", L"*.*" }
            };
            ThrowIfFailed(
                dialog->SetFileTypes(static_cast<UINT>(std::size(filters)), filters),
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
                g_directX.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)),
                "スワップチェーンのバックバッファを取得できません。");
            ThrowIfFailed(
                g_directX.device->CreateRenderTargetView(
                    backBuffer.Get(),
                    nullptr,
                    &g_directX.renderTargetView),
                "描画先を作成できません。");
        }

        void CreateDirectXDevice(HWND window)
        {
            DXGI_SWAP_CHAIN_DESC swapChainDescription{};
            swapChainDescription.BufferCount = kSwapChainBufferCount;
            swapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDescription.OutputWindow = window;
            swapChainDescription.SampleDesc.Count = 1;
            swapChainDescription.Windowed = TRUE;
            swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            constexpr D3D_FEATURE_LEVEL requestedFeatureLevels[] = {
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_0
            };
            constexpr D3D_DRIVER_TYPE driverTypes[] = {
                D3D_DRIVER_TYPE_HARDWARE,
                D3D_DRIVER_TYPE_WARP
            };

            HRESULT createResult = E_FAIL;
            D3D_FEATURE_LEVEL createdFeatureLevel{};
            for (const D3D_DRIVER_TYPE driverType : driverTypes)
            {
                createResult = D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    driverType,
                    nullptr,
                    0,
                    requestedFeatureLevels,
                    static_cast<UINT>(std::size(requestedFeatureLevels)),
                    D3D11_SDK_VERSION,
                    &swapChainDescription,
                    &g_directX.swapChain,
                    &g_directX.device,
                    &createdFeatureLevel,
                    &g_directX.context);
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
                std::filesystem::path(windowsDirectory) / L"Fonts" / kJapaneseFontFileName;
            if (!std::filesystem::exists(fontPath))
            {
                return;
            }

            const std::string fontPathUtf8 = PathToUtf8(fontPath);
            ImGuiIO& inputOutput = ImGui::GetIO();
            inputOutput.Fonts->AddFontFromFileTTF(
                fontPathUtf8.c_str(),
                kJapaneseFontSize,
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

            constexpr ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse;

            ImGui::Begin("StageConverterMainWindow", nullptr, windowFlags);
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
                kPixelsPerUnitStep,
                kPixelsPerUnitFastStep,
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
                if (g_directX.swapChain && wordParameter != SIZE_MINIMIZED)
                {
                    g_directX.renderTargetView.Reset();
                    const UINT width = static_cast<UINT>(LOWORD(longParameter));
                    const UINT height = static_cast<UINT>(HIWORD(longParameter));
                    if (SUCCEEDED(g_directX.swapChain->ResizeBuffers(
                        0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
                    {
                        try
                        {
                            CreateRenderTarget();
                        }
                        catch (const std::exception&)
                        {
                            PostQuitMessage(kExitInitializationFailure);
                        }
                    }
                }
                return 0;

            case WM_SYSCOMMAND:
                if ((wordParameter & kSystemCommandMask) == SC_KEYMENU)
                {
                    return 0;
                }
                break;

            case WM_DESTROY:
                PostQuitMessage(kExitSuccess);
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
                kWindowTitle,
                MB_OK | MB_ICONERROR);
            return kExitInitializationFailure;
        }

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_CLASSDC;
        windowClass.lpfnWndProc = WindowProcedure;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = kWindowClassName;

        if (RegisterClassExW(&windowClass) == 0)
        {
            CoUninitialize();
            return kExitInitializationFailure;
        }

        const HWND window = CreateWindowW(
            kWindowClassName,
            kWindowTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            kInitialWindowWidth,
            kInitialWindowHeight,
            nullptr,
            nullptr,
            instance,
            nullptr);
        if (window == nullptr)
        {
            UnregisterClassW(kWindowClassName, instance);
            CoUninitialize();
            return kExitInitializationFailure;
        }
        g_mainWindow = window;

        try
        {
            CreateDirectXDevice(window);
        }
        catch (const std::exception& error)
        {
            const std::wstring wideMessage = Utf8ToWide(error.what());
            MessageBoxW(window, wideMessage.c_str(), kWindowTitle, MB_OK | MB_ICONERROR);
            DestroyWindow(window);
            UnregisterClassW(kWindowClassName, instance);
            CoUninitialize();
            return kExitInitializationFailure;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        LoadJapaneseFont();
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX11_Init(g_directX.device.Get(), g_directX.context.Get());

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

            g_directX.context->OMSetRenderTargets(
                1,
                g_directX.renderTargetView.GetAddressOf(),
                nullptr);
            g_directX.context->ClearRenderTargetView(
                g_directX.renderTargetView.Get(),
                kClearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_directX.swapChain->Present(kPresentSyncInterval, 0);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        g_directX.renderTargetView.Reset();
        g_directX.swapChain.Reset();
        g_directX.context.Reset();
        g_directX.device.Reset();

        DestroyWindow(window);
        g_mainWindow = nullptr;
        UnregisterClassW(kWindowClassName, instance);
        CoUninitialize();
        return kExitSuccess;
    }
}
