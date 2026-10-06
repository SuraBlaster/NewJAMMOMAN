#include <windows.h>
#include <DirectXTex.h>
#include <cassert>
#include <cstdio>
#include "Sprite.h"
#include "RenderState.h"
#include "HealthGaugeTrail.h"

int main()
{
    HealthGaugeTrail trail;
    trail.Update(0.5f, 0.016f);
    assert(trail.current == 0.5f && trail.trailing == 1.0f);
    trail.Update(0.5f, 0.2f);
    assert(trail.trailing == 1.0f);
    trail.Update(0.5f, 0.3f);
    assert(trail.trailing < 1.0f && trail.trailing > 0.5f);
    trail.Update(0.25f, 0.016f);
    const float repeatedHit = trail.trailing;
    trail.Update(0.25f, 0.1f);
    assert(trail.trailing == repeatedHit);
    trail.Update(0.25f, 2.0f);
    assert(trail.trailing == 0.25f);
    trail.Update(1.0f, 0.016f);
    assert(trail.current == 1.0f && trail.trailing == 1.0f);
    trail.Update(0.0f, 0.016f);
    trail.Update(0.0f, 3.0f);
    assert(trail.trailing == 0.0f);
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> dc;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &dc)));
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1280; desc.Height = 720; desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> target;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> view;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target)));
    assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &view)));
    dc->OMSetRenderTargets(1, view.GetAddressOf(), nullptr);
    D3D11_VIEWPORT viewport = { 0, 0, 1280, 720, 0, 1 };
    dc->RSSetViewports(1, &viewport);
    RenderState states(device.Get());
    dc->RSSetState(states.GetRasterizerState(RasterizerState::SolidCullNone));
    dc->OMSetDepthStencilState(states.GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->OMSetBlendState(states.GetBlendState(BlendState::Transparency), nullptr, ~0u);
    auto* sampler = states.GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);
    D3D11_BUFFER_DESC buffer = {};
    buffer.ByteWidth = 48; buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants;
    assert(SUCCEEDED(device->CreateBuffer(&buffer, nullptr, &constants)));
    dc->PSSetConstantBuffers(1, 1, constants.GetAddressOf());
    Sprite player(device.Get(), "Data/Sprite/PlayerGauge.png", true, "Data/Shader/HealthGaugePS.cso");
    Sprite boss(device.Get(), "Data/Sprite/EnemyGauge.png", true, "Data/Shader/HealthGaugePS.cso");
    for (int step = 0; step <= 3; ++step)
    {
        const float background[] = { 0.15f, 0.18f, 0.22f, 1 };
        dc->ClearRenderTargetView(view.Get(), background);
        DirectX::XMFLOAT4 data[] = {
            {260.f/807,80.f/1949,535.f/807,1270.f/1949},
            {0.12f,0.95f,0.28f,1}, {step == 3 ? 0.5f : step * 0.5f,step == 3 ? 1.0f : 0.0f,0,0}};
        dc->UpdateSubresource(constants.Get(), 0, nullptr, data, 0, 0);
        player.Render(dc.Get(), 24,24,0,300.f*807/1949,300,0,1,1,1,1);
        data[0] = {310.f/724,95.f/2171,635.f/724,1405.f/2171};
        data[1] = {0.72f,0.20f,1,1};
        dc->UpdateSubresource(constants.Get(), 0, nullptr, data, 0, 0);
        boss.Render(dc.Get(), 1256-300.f*724/2171,24,0,300.f*724/2171,300,0,1,1,1,1);
        DirectX::ScratchImage capture;
        assert(SUCCEEDED(DirectX::CaptureTexture(device.Get(), dc.Get(), target.Get(), capture)));
        const auto* im = capture.GetImage(0,0,0);
        for (int side = 0; side < 2; ++side)
        {
            const int x = side ? 1220 : 85;
            for (int row = 0; row < 2; ++row)
            {
                const auto* pixel = im->pixels + (row ? 180 : 60) * im->rowPitch + x * 4;
                const bool filled = step == 2 || ((step == 1 || step == 3) && row == 1);
                if (filled) assert(side ? pixel[2] > pixel[1]*2 : pixel[1] > pixel[0]*2);
                else if (step == 3) assert(pixel[0] > pixel[1]*3 && pixel[0] > pixel[2]*3);
                else assert(pixel[0] < 25 && pixel[1] < 25 && pixel[2] < 25);
            }
        }
        wchar_t filename[80];
        swprintf_s(filename, L"obj/health-gauges-%d.png", step);
        assert(SUCCEEDED(DirectX::SaveToWICFile(*im, DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), filename)));
    }
    puts("PASS: damage delay, repeated hits, recovery, death; green/purple fills over red trails; top-to-bottom depletion.");
}
