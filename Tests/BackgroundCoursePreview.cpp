#include <windows.h>
#include <DirectXTex.h>
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "Framework.h"
#include "GameScene.h"
#include "Graphics.h"
#include "ImGuiRenderer.h"
#include "Camera.h"

static void Check(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("Background preview failed");
    }
}

int main(int argc, char** argv)
{
    const std::string tag = argc > 1 ? argv[1] : "after";
    const auto directory = std::filesystem::path("output/stage-design/background-" + tag);
    std::filesystem::create_directories(directory);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"BackgroundCoursePreview";
    RegisterClassW(&windowClass);
    HWND window = CreateWindowW(windowClass.lpszClassName, L"Background preview", WS_POPUP,
        0, 0, 1280, 720, nullptr, nullptr, windowClass.hInstance, nullptr);

    try
    {
        Framework framework(window);
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        GameScene scene;
        scene.Initialize();
        auto& graphics = Graphics::Instance();

        // Publish placed model transforms and finish the player's spawn pose.
        for (int frame = 0; frame < 120; ++frame)
        {
            ImGuiRenderer::NewFrame();
            scene.Update(1.0f / 60.0f);
            ImGui::EndFrame();
        }

        const DirectX::XMFLOAT2 views[] = {
            { 8, 3 }, { 87, 13 }, { 113, 6 }, { 124, 18 },
            { 168, 29 }, { 183, 18 }, { 218, 25 }, { 256, 27 }, { 271, 28 }
        };

        for (size_t index = 0; index < std::size(views); ++index)
        {
            const auto point = views[index];
            Camera::Instance().SetLookAt({ point.x, point.y + 1, -10 }, { point.x, point.y, 0 }, { 0, 1, 0 });
            ImGuiRenderer::NewFrame();
            graphics.SetRenderTargets();
            scene.Render(1.0f / 60.0f);
            ImGuiRenderer::Render(graphics.GetDeviceContext());

            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
            graphics.GetDeviceContext()->OMGetRenderTargets(1, &target, nullptr);
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            target->GetResource(&resource);
            DirectX::ScratchImage capture;
            Check(DirectX::CaptureTexture(graphics.GetDevice(), graphics.GetDeviceContext(), resource.Get(), capture));
            const auto destination = directory / (std::to_string(index + 1) + ".png");
            Check(DirectX::SaveToWICFile(*capture.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE,
                DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), destination.c_str()));
        }

        std::cout << "Captured 9 course views: " << directory.string() << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    DestroyWindow(window);
    CoUninitialize();
}
