#include <windows.h>
#include <imgui.h>
#include <DirectXTex.h>
#include "Graphics.h"
#include "ImGuiRenderer.h"
#include "SceneTitle.h"
#include "TitleAnimation.h"
#include <cassert>
#include <cstdio>

int main()
{
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    assert(SUCCEEDED(com));
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TitleRenderTest";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Title render test", WS_POPUP,
        0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    assert(window);
    auto& graphics = Graphics::Instance();
    graphics.Initialize(window);
    ImGuiRenderer::Initialize(window, graphics.GetDevice(), graphics.GetDeviceContext());
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().IniFilename = nullptr;
    SceneTitle title;
    title.Initialize();
    // Simulate two ten-second loading/stall frames. They must not skip the intro.
    for (int frame = 0; frame < 2; ++frame)
    {
        ImGuiRenderer::NewFrame();
        title.Update(10.0f);
        graphics.SetRenderTargets();
        title.Render(0);
        ImGuiRenderer::Render(graphics.GetDeviceContext());
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        graphics.GetDeviceContext()->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        target->GetResource(resource.GetAddressOf());
        DirectX::ScratchImage capture;
        assert(SUCCEEDED(DirectX::CaptureTexture(graphics.GetDevice(), graphics.GetDeviceContext(), resource.Get(), capture)));
        const auto* image = capture.GetImage(0, 0, 0);
        size_t visiblePixels = 0;
        for (size_t y = 0; y < image->height; ++y)
            for (size_t x = 0; x < image->width; ++x)
            {
                const auto* pixel = image->pixels + y * image->rowPitch + x * 4;
                if (pixel[0] > 30 || pixel[1] > 30 || pixel[2] > 30) ++visiblePixels;
            }
        if (frame == 0) assert(visiblePixels == 0);
        else assert(visiblePixels > 0 && visiblePixels < 5000);
    }
    puts("Long loading frames do not skip the title intro: PASS");
    const float times[] = {0.4f, 0.8f, 2.05f, 2.35f, 2.55f, 2.9f, 3.4f, 4.2f, 4.8f, 14.8f};
    float previous = 0.1f;
    for (int index = 0; index < 10; ++index)
    {
        ImGuiRenderer::NewFrame();
        float remaining = times[index] - previous;
        while (remaining > 0.000001f)
        {
            const float step = (std::min)(remaining, 1.0f / 60.0f);
            title.Update(step);
            remaining -= step;
        }
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
        swprintf_s(filename, L"obj/title-preview-%02d.png", index);
        hr = DirectX::SaveToWICFile(*capture.GetImage(0,0,0), DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), filename);
        assert(SUCCEEDED(hr));
        printf("Rendered %.2f seconds\n", times[index]);
    }
    // The title actor retains the original one-second delay before jumping.
    TitlePlayer actor;
    const float ground = actor.GetPosition().y;
    actor.BeginStart();
    actor.Update(0.9f);
    assert(actor.GetPosition().y == ground);
    actor.Update(0.2f);
    assert(actor.GetPosition().y > ground);
    actor.Reset();
    assert(actor.GetPosition().y == ground);
    title.Finalize();
    ImGuiRenderer::Finalize();
    DestroyWindow(window);
    CoUninitialize();
}
