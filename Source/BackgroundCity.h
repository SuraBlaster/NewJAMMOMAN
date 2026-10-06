#pragma once
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <imgui.h>
#include "Graphics.h"
#include "Camera.h"
#include "GpuResourceUtils.h"

// Draw before opaque world geometry; wall openings naturally reveal the skyline.
class BackgroundCity
{
public:
    explicit BackgroundCity(ID3D11Device* device)
    {
        std::ifstream input("Data/Shader/FogVS.cso",std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)),{});
        Check(device->CreateVertexShader(bytes.data(),bytes.size(),nullptr,vs.GetAddressOf()));
        Check(GpuResourceUtils::LoadPixelShader(device,"Data/Shader/CityPS.cso",ps.GetAddressOf()));
        Check(GpuResourceUtils::CreateConstantBuffer(device,sizeof(Constants),buffer.GetAddressOf()));
    }
    void Update(float dt) { time+=(std::max)(0.0f,dt); }
    void DrawGUI()
    {
        if(ImGui::Begin("Distant City"))
        {
            ImGui::Checkbox("Enabled",&enabled);
            ImGui::SliderFloat("Brightness",&brightness,0.2f,2.0f);
            ImGui::SliderFloat("Window lights",&lights,0,2);
            ImGui::SliderFloat("Skyline height",&horizon,0.05f,0.5f);
            ImGui::SliderFloat("Building scale",&scale,0.5f,1.5f);
            ImGui::TextUnformatted("Three skyline layers / slow camera parallax");
            if(ImGui::Button("Reset")){brightness=1;lights=1;horizon=0.24f;scale=1;}
        }
        ImGui::End();
    }
    void Render()
    {
        if(!enabled)return;
        auto& g=Graphics::Instance();auto* dc=g.GetDeviceContext();auto* states=g.GetRenderState();
        UINT count=1;D3D11_VIEWPORT viewport;dc->RSGetViewports(&count,&viewport);
        if(!count||viewport.Height<=0)return;
        const auto& focus=Camera::Instance().GetFocus();
        Constants data={{time,viewport.Width/viewport.Height,brightness,lights},
            {focus.x*0.015f,std::clamp(-focus.y*0.0015f,-0.10f,0.10f),horizon,scale}};
        dc->UpdateSubresource(buffer.Get(),0,nullptr,&data,0,0);
        auto* cb=buffer.Get();dc->PSSetConstantBuffers(0,1,&cb);
        dc->IASetInputLayout(nullptr);dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        dc->VSSetShader(vs.Get(),nullptr,0);dc->GSSetShader(nullptr,nullptr,0);dc->PSSetShader(ps.Get(),nullptr,0);
        dc->RSSetState(states->GetRasterizerState(RasterizerState::SolidCullNone));
        dc->OMSetBlendState(states->GetBlendState(BlendState::Opaque),nullptr,0xffffffff);
        dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::NoTestNoWrite),0);
        dc->Draw(3,0);
        dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::TestAndWrite),0);
    }
private:
    struct Constants{DirectX::XMFLOAT4 settings,view;};
    static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("City background GPU resource creation failed");}
    bool enabled=true;
    float time=0,brightness=1,lights=1,horizon=0.24f,scale=1;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
};
