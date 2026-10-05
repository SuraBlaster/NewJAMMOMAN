#pragma once
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <imgui.h>
#include "Graphics.h"
#include "Camera.h"
#include "GpuResourceUtils.h"

// Half-resolution procedural mist, composited at a depth behind gameplay.
class BackgroundFog
{
public:
    explicit BackgroundFog(ID3D11Device* device)
    {
        std::ifstream input("Data/Shader/FogVS.cso", std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        Check(device->CreateVertexShader(bytes.data(), bytes.size(), nullptr, vs.GetAddressOf()));
        Check(GpuResourceUtils::LoadPixelShader(device, "Data/Shader/FogPS.cso", ps.GetAddressOf()));
        Check(GpuResourceUtils::LoadPixelShader(device, "Data/Shader/FogCompositePS.cso", composite.GetAddressOf()));
        Check(GpuResourceUtils::CreateConstantBuffer(device, sizeof(Constants), buffer.GetAddressOf()));
    }
    void Update(float dt) { time += (std::max)(0.0f, dt); }
    void DrawGUI()
    {
        if (ImGui::Begin("Background Fog"))
        {
            ImGui::Checkbox("Enabled", &enabled);
            ImGui::SliderFloat("Density", &density, 0, 0.65f);
            ImGui::SliderFloat("Speed", &speed, 0, 0.25f);
            ImGui::SliderFloat("Cloud scale", &scale, 1, 8);
            ImGui::ColorEdit3("Color", &color.x);
            ImGui::TextUnformatted("Half resolution / behind gameplay objects");
            if (ImGui::Button("Reset")) { density = 0.24f; speed = 0.045f; scale = 3.5f; color = {0.34f, 0.48f, 0.52f}; }
        }
        ImGui::End();
    }
    void Render()
    {
        if (!enabled || density <= 0) return;
        auto& g = Graphics::Instance();
        auto* dc = g.GetDeviceContext();
        auto* states = g.GetRenderState();
        UINT count = 1; D3D11_VIEWPORT viewport;
        dc->RSGetViewports(&count, &viewport);
        if (!count || viewport.Width < 1 || viewport.Height < 1) return;
        const UINT w = (std::max)(1u, UINT(viewport.Width) / 2), h = (std::max)(1u, UINT(viewport.Height) / 2);
        if (w != width || h != height) Resize(g.GetDevice(), w, h);
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth;
        dc->OMGetRenderTargets(1, target.GetAddressOf(), depth.GetAddressOf());
        auto& camera = Camera::Instance();
        const auto focus = camera.GetFocus();
        auto point = DirectX::XMVectorSet(focus.x, focus.y, 4.0f, 1);
        point = DirectX::XMVector4Transform(point, DirectX::XMLoadFloat4x4(&camera.GetView()) * DirectX::XMLoadFloat4x4(&camera.GetProjection()));
        const float clipW = DirectX::XMVectorGetW(point);
        if (clipW <= 0) return;
        Constants data = {{time, viewport.Width / viewport.Height, density, speed},
            {color.x, color.y, color.z, 1}, {focus.x * 0.018f, -focus.y * 0.018f, scale,
            std::clamp(DirectX::XMVectorGetZ(point) / clipW, 0.0f, 1.0f)}};
        dc->UpdateSubresource(buffer.Get(), 0, nullptr, &data, 0, 0);
        ID3D11Buffer* cb = buffer.Get(); dc->PSSetConstantBuffers(0, 1, &cb);
        dc->IASetInputLayout(nullptr);
        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        dc->VSSetShader(vs.Get(), nullptr, 0);
        dc->GSSetShader(nullptr, nullptr, 0);
        dc->RSSetState(states->GetRasterizerState(RasterizerState::SolidCullNone));
        ID3D11RenderTargetView* offscreen = rtv.Get();
        dc->OMSetRenderTargets(1, &offscreen, nullptr);
        D3D11_VIEWPORT low = {0, 0, float(w), float(h), 0, 1}; dc->RSSetViewports(1, &low);
        dc->OMSetBlendState(states->GetBlendState(BlendState::Opaque), nullptr, 0xffffffff);
        dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
        dc->PSSetShader(ps.Get(), nullptr, 0); dc->Draw(3, 0);
        ID3D11RenderTargetView* original = target.Get();
        dc->OMSetRenderTargets(1, &original, depth.Get()); dc->RSSetViewports(1, &viewport);
        dc->OMSetBlendState(states->GetBlendState(BlendState::Transparency), nullptr, 0xffffffff);
        dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::TestOnly), 0);
        auto* sampler = states->GetSamplerState(SamplerState::LinearClamp); dc->PSSetSamplers(0, 1, &sampler);
        ID3D11ShaderResourceView* view = srv.Get(); dc->PSSetShaderResources(0, 1, &view);
        dc->PSSetShader(composite.Get(), nullptr, 0); dc->Draw(3, 0);
        view = nullptr; dc->PSSetShaderResources(0, 1, &view);
        dc->OMSetBlendState(states->GetBlendState(BlendState::Opaque), nullptr, 0xffffffff);
        dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::TestAndWrite), 0);
    }
private:
    struct Constants { DirectX::XMFLOAT4 parameters, tint, placement; };
    static void Check(HRESULT hr) { if (FAILED(hr)) throw std::runtime_error("Background fog GPU resource creation failed"); }
    void Resize(ID3D11Device* device, UINT w, UINT h)
    {
        rtv.Reset(); srv.Reset();
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w; desc.Height = h; desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; desc.SampleDesc.Count = 1;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Check(device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf()));
        Check(device->CreateRenderTargetView(texture.Get(), nullptr, rtv.GetAddressOf()));
        Check(device->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf()));
        width = w; height = h;
    }
    bool enabled = true;
    float time = 0, density = 0.24f, speed = 0.045f, scale = 3.5f;
    DirectX::XMFLOAT3 color = {0.34f, 0.48f, 0.52f};
    UINT width = 0, height = 0;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps, composite;
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
};
