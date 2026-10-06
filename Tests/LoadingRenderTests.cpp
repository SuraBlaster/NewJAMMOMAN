#include <windows.h>
#include <imgui.h>
#include <DirectXTex.h>
#include "Graphics.h"
#include "ImGuiRenderer.h"
#include "SceneLoading.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

int main()
{
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    assert(SUCCEEDED(com));
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"LoadingRenderTest";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Loading render test", WS_POPUP,
        0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    assert(window);
    auto& graphics = Graphics::Instance();
    graphics.Initialize(window);
    ImGuiRenderer::Initialize(window, graphics.GetDevice(), graphics.GetDeviceContext());
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().IniFilename = nullptr;
    int transitions = 0;
    SceneLoading title([&transitions]() { ++transitions; return std::make_shared<Scene>(); }, true);
    title.Initialize();
    // Simulate a long model load, followed by a slow first displayed frame.
    title.Update(10.0f);
    ImGuiRenderer::NewFrame();
    // The UI keeps Japanese glyphs while game text uses a separate font.
    assert(ImGui::GetIO().Fonts->Fonts.Size == 2);
    assert(ImGui::GetFont() == ImGui::GetIO().FontDefault);
    assert(ImGui::GetFont() != ImGui::GetIO().Fonts->Fonts[1]);
    assert(ImGui::GetFont()->FindGlyphNoFallback(0x65e5));
    assert(ImGui::GetFont()->FindGlyphNoFallback(0x672c));
    assert(ImGui::GetFont()->FindGlyphNoFallback(0x8a9e));
    title.DrawGUI();
    ImGuiRenderer::Render(graphics.GetDeviceContext());
    title.Update(10.0f);
    SceneManager::Instance().Update(0);
    assert(transitions == 0);
    const auto advance = [&](float duration)
    {
        while (duration > 0.00001f)
        {
            const float step = (std::min)(duration, 1.0f / 60);
            ImGuiRenderer::NewFrame();
            title.Update(step);
            title.DrawGUI();
            ImGuiRenderer::Render(graphics.GetDeviceContext());
            duration -= step;
        }
    };
    const float times[] = {0.0f, 0.66f, 1.2f, 1.8f, 2.4f, 3.0f, 3.4f, 4.0f, 4.5f};
    float previous = 0;
    for (int index = 0; index < 9; ++index)
    {
        advance(times[index] - previous);
        ImGuiRenderer::NewFrame();
        previous = times[index];
        graphics.Clear(0,0,0,1);
        graphics.SetRenderTargets();
        title.Render(0);
        title.DrawGUI();
        ImGuiRenderer::Render(graphics.GetDeviceContext());
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        graphics.GetDeviceContext()->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        target->GetResource(resource.GetAddressOf());
        DirectX::ScratchImage capture;
        HRESULT hr = DirectX::CaptureTexture(graphics.GetDevice(), graphics.GetDeviceContext(), resource.Get(), capture);
        assert(SUCCEEDED(hr));
        wchar_t filename[128];
        swprintf_s(filename, L"obj/loading-preview-%02d.png", index);
        hr = DirectX::SaveToWICFile(*capture.GetImage(0,0,0), DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), filename);
        assert(SUCCEEDED(hr));
        printf("Rendered %.2f seconds\n", times[index]);
    }
    SceneManager::Instance().Update(0);
    assert(transitions == 0);
    advance(1.0f);
    SceneManager::Instance().Update(0);
    assert(transitions == 1);
    advance(1.0f);
    SceneManager::Instance().Update(0);
    assert(transitions == 1);
    SceneManager::Instance().Clear();
    title.Finalize();
    ImGuiRenderer::Finalize();
    DestroyWindow(window);
    CoUninitialize();
}
