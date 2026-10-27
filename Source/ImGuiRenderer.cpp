#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ImGuizmo.h>
#include "ImGuiRenderer.h"
#include "TextRenderer.h"
#include <stdexcept>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

// 初期化
void ImGuiRenderer::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc)
{

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

#ifdef _DEBUG
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#else
    io.IniFilename = nullptr;
#endif

#if 1
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
#endif

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(device, dc);

    io.FontDefault = io.Fonts->AddFontFromFileTTF("Data/Font/ArialUni.ttf", 18.0f, nullptr,
                                                  io.Fonts->GetGlyphRangesJapanese());

    if (!io.FontDefault)
    {
        throw std::runtime_error("Failed to initialize Japanese ImGui font.");
    }

    if (!TextRenderer::Initialize("Data/Font/Oxanium-ExtraBold.ttf", 18.0f))
    {
        throw std::runtime_error("Failed to initialize TextRenderer.");
    }
}

// 終了化
void ImGuiRenderer::Finalize()
{
    TextRenderer::Finalize();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// フレーム開始処理
void ImGuiRenderer::NewFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
    ImVec2 size = ImGui::GetIO().DisplaySize;
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

#if 0

    const ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_None | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    const ImGuiDockNodeFlags docspaceFlags = ImGuiDockNodeFlags_None

                                             | ImGuiDockNodeFlags_PassthruCentralNode;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    bool dockOpen = true;

    if (ImGui::Begin("MainDockspace", &dockOpen, windowFlags))
    {
        ImGui::PopStyleVar(3);

        ImGuiID dockspaceId = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0, 0), docspaceFlags);
    }

    ImGui::End();
#endif
}

// 描画
void ImGuiRenderer::Render(ID3D11DeviceContext* context)
{

    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

// WIN32メッセージハンドラー
LRESULT ImGuiRenderer::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
