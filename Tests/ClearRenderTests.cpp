#include <windows.h>
#include <imgui.h>
#include <DirectXTex.h>
#include "Graphics.h"
#include "ImGuiRenderer.h"
#include "SceneClear.h"
#include "Audio/Audio.h"
#include "SceneManager.h"
#include <cassert>
#include <cstdio>

class ObservedClear : public SceneClear
{
public:
    explicit ObservedClear(bool& finished) : finished(finished) {}
    void Finalize() override { finished = true; SceneClear::Finalize(); }
private:
    bool& finished;
};

int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"ClearRenderTest";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Clear test", WS_POPUP,
        0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    auto& graphics = Graphics::Instance();
    graphics.Initialize(window);
    ImGuiRenderer::Initialize(window, graphics.GetDevice(), graphics.GetDeviceContext());
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().IniFilename = nullptr;
    {
        ClearPlayer player;
        assert(player.GetModel()->GetAnimationIndex("Wave") >= 0);
        player.Update(20);
        assert(!player.IsWaving());
        player.BeginEntrance();
        player.Update(10);
        assert(player.IsWaving());
        assert(player.GetPosition().x == 0);
        player.Update(10);
        assert(player.GetPosition().x == 0);
    }
    Audio audio;
    bool finished = false;
    auto& manager = SceneManager::Instance();
    manager.ChangeScene(std::make_shared<ObservedClear>(finished));
    manager.Update(30); // Loading time must not skip the sequence.
    for (int frame = 0; frame < 640; ++frame)
    {
        ImGuiRenderer::NewFrame();
        manager.Update(1.0f / 60);
        graphics.SetRenderTargets();
        manager.Render(0);
        manager.DrawGUI();
        ImGuiRenderer::Render(graphics.GetDeviceContext());
        if (frame == 60 || frame == 230 || frame == 400)
        {
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
            graphics.GetDeviceContext()->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            target->GetResource(resource.GetAddressOf());
            DirectX::ScratchImage capture;
            assert(SUCCEEDED(DirectX::CaptureTexture(graphics.GetDevice(), graphics.GetDeviceContext(), resource.Get(), capture)));
            wchar_t filename[128];
            swprintf_s(filename, L"obj/clear-preview-%03d.png", frame);
            assert(SUCCEEDED(DirectX::SaveToWICFile(*capture.GetImage(0,0,0), DirectX::WIC_FLAGS_NONE,
                DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), filename)));
        }
        if (frame < 550) assert(!finished);
    }
    assert(finished);
    manager.Clear();
    ImGuiRenderer::Finalize();
    DestroyWindow(window);
    CoUninitialize();
    puts("Clear entrance, wave, rendering and automatic transition passed.");
}
