#include <windows.h>
#include <d3d11sdklayers.h>
#include <imgui.h>
#include <atomic>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include "Framework.h"
#include "Graphics.h"
#include "EnemyMage.h"
#include "Player.h"
#include "Effect/EffectManager.h"

int main()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"EnemyMageLoadingTest";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Loading test", WS_POPUP, 0, 0, 1280, 720, nullptr,
                                nullptr, wc.hInstance, nullptr);
    int result = 0;

    try
    {
        Framework framework(window);
        ImGui::GetIO().IniFilename = nullptr;
        auto& graphics = Graphics::Instance();
        Player player(graphics.GetDevice());
        Microsoft::WRL::ComPtr<ID3D11InfoQueue> messages;

        if (FAILED(graphics.GetDevice()->QueryInterface(IID_PPV_ARGS(&messages))))
        {
            throw std::runtime_error("D3D11 debug layer required");
        }

        messages->ClearStoredMessages();
        std::vector<std::unique_ptr<EnemyMage>> mages;
        std::exception_ptr error;
        std::atomic<bool> done = false;
        std::thread loader(
            [&]
            {
                const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

                try
                {
                    if (FAILED(com))
                    {
                        throw std::runtime_error("Worker COM failed");
                    }

                    for (int i = 0; i < 8; ++i)
                    {
                        mages.push_back(std::make_unique<EnemyMage>(graphics.GetDevice(), &player));
                    }
                }
                catch (...)
                {
                    error = std::current_exception();
                }

                if (SUCCEEDED(com))
                {
                    CoUninitialize();
                }

                done.store(true);
            });

        while (!done.load())
        {
            graphics.Clear(0, 0, 0, 1);
            EffectManager::Instance().Update(1.0f / 60.0f);
            EffectManager::Instance().Render(Camera::Instance().GetView(),
                                             Camera::Instance().GetProjection());
        }

        loader.join();

        if (error)
        {
            std::rethrow_exception(error);
        }

        mages.front()->Update(0.0f);
        mages.front()->Update(1.6f);
        EffectManager::Instance().Update(1.0f / 60.0f);

        if (EffectManager::Instance().GetEffekseerManager()->GetTotalInstanceCount() == 0)
        {
            throw std::runtime_error("Deferred Mage effect did not play");
        }

        for (UINT64 i = 0; i < messages->GetNumStoredMessages(); ++i)
        {
            SIZE_T size = 0;
            messages->GetMessage(i, nullptr, &size);
            std::vector<char> buffer(size);
            auto* message = reinterpret_cast<D3D11_MESSAGE*>(buffer.data());
            messages->GetMessage(i, message, &size);

            if (message->Severity <= D3D11_MESSAGE_SEVERITY_ERROR)
            {
                throw std::runtime_error(message->pDescription);
            }
        }

        std::cout
            << "PASS: worker Mage construction with concurrent clear/effect rendering; main-thread effect load and firing; no D3D11 errors\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        result = 1;
    }

    DestroyWindow(window);
    CoUninitialize();
    return result;
}
