#include <algorithm>
#include <imgui.h>
#include "Misc.h"
#include "GpuResourceUtils.h"
#include "Bloom.h"

Bloom::Bloom(ID3D11Device* device, UINT width, UINT height) : sceneWidth(width), sceneHeight(height)
{
    // 頂点はシェーダー内で作るため入力レイアウトは使わない
    GpuResourceUtils::LoadVertexShader(device, "Data/Shader/BloomVS.cso", nullptr, 0, nullptr,
                                       vertexShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomPrefilterPS.cso",
                                      prefilterShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomDownsamplePS.cso",
                                      downsampleShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomUpsamplePS.cso",
                                      upsampleShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomCompositePS.cso",
                                      compositeShader.GetAddressOf());
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbBloom), constantBuffer.GetAddressOf());

    // 半分の解像度から始めて段階的に縮小したバッファ
    UINT mipWidth = width;
    UINT mipHeight = height;

    for (MipTarget& mip : mips)
    {
        mipWidth = (std::max)(1u, mipWidth / 2);
        mipHeight = (std::max)(1u, mipHeight / 2);

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = mipWidth;
        desc.Height = mipHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        HRESULT hr = device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        hr = device->CreateRenderTargetView(texture.Get(), nullptr, mip.renderTargetView.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        hr = device->CreateShaderResourceView(texture.Get(), nullptr, mip.shaderResourceView.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        mip.width = mipWidth;
        mip.height = mipHeight;
    }

    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        HRESULT hr = device->CreateSamplerState(&desc, linearClampSampler.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }
    {
        D3D11_BLEND_DESC desc{};
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRESULT hr = device->CreateBlendState(&desc, opaqueBlendState.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

        // 拡大しながら一段大きいバッファへ足し込むため
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        hr = device->CreateBlendState(&desc, additiveBlendState.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }
    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        HRESULT hr = device->CreateDepthStencilState(&desc, noDepthState.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }
    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.DepthClipEnable = true;
        HRESULT hr = device->CreateRasterizerState(&desc, rasterizerState.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }
}

void Bloom::DrawFullscreen(ID3D11DeviceContext* dc, ID3D11PixelShader* pixelShader,
                           ID3D11ShaderResourceView* source, UINT sourceWidth, UINT sourceHeight,
                           ID3D11RenderTargetView* target, UINT targetWidth, UINT targetHeight)
{
    CbBloom cb{};
    cb.texelSize[0] = 1.0f / static_cast<float>(sourceWidth);
    cb.texelSize[1] = 1.0f / static_cast<float>(sourceHeight);
    cb.threshold = threshold;
    cb.knee = knee;
    cb.intensity = intensity;
    dc->UpdateSubresource(constantBuffer.Get(), 0, 0, &cb, 0, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(targetWidth);
    viewport.Height = static_cast<float>(targetHeight);
    viewport.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &viewport);

    // 書き込み先を前の工程で読んでいた場合に備えて、先にSRVを外す
    ID3D11ShaderResourceView* nullSrv = nullptr;
    dc->PSSetShaderResources(0, 1, &nullSrv);
    dc->OMSetRenderTargets(1, &target, nullptr);
    dc->PSSetShader(pixelShader, nullptr, 0);
    dc->PSSetShaderResources(0, 1, &source);
    dc->Draw(3, 0);
}

void Bloom::Render(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* scene, ID3D11RenderTargetView* target,
                   const D3D11_VIEWPORT& viewport)
{
    dc->IASetInputLayout(nullptr);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
    dc->PSSetSamplers(0, 1, linearClampSampler.GetAddressOf());
    dc->OMSetDepthStencilState(noDepthState.Get(), 0);
    dc->RSSetState(rasterizerState.Get());
    dc->OMSetBlendState(opaqueBlendState.Get(), nullptr, 0xFFFFFFFF);

    if (enabled)
    {
        // 明るい部分の抽出と縮小
        DrawFullscreen(dc, prefilterShader.Get(), scene, sceneWidth, sceneHeight,
                       mips[0].renderTargetView.Get(), mips[0].width, mips[0].height);

        for (int i = 1; i < MIP_COUNT; i++)
        {
            DrawFullscreen(dc, downsampleShader.Get(), mips[i - 1].shaderResourceView.Get(),
                           mips[i - 1].width, mips[i - 1].height, mips[i].renderTargetView.Get(),
                           mips[i].width, mips[i].height);
        }

        // 小さいバッファから順に拡大して足し込む
        dc->OMSetBlendState(additiveBlendState.Get(), nullptr, 0xFFFFFFFF);

        for (int i = MIP_COUNT - 1; i > 0; i--)
        {
            DrawFullscreen(dc, upsampleShader.Get(), mips[i].shaderResourceView.Get(), mips[i].width,
                           mips[i].height, mips[i - 1].renderTargetView.Get(), mips[i - 1].width,
                           mips[i - 1].height);
        }

        dc->OMSetBlendState(opaqueBlendState.Get(), nullptr, 0xFFFFFFFF);
    }

    // シーンとブルームを合成してバックバッファへ
    ID3D11ShaderResourceView* nullSrvs[2] = {};
    dc->PSSetShaderResources(0, 2, nullSrvs);
    dc->RSSetViewports(1, &viewport);
    dc->OMSetRenderTargets(1, &target, nullptr);
    CbBloom cb{};
    cb.texelSize[0] = 1.0f / static_cast<float>(mips[0].width);
    cb.texelSize[1] = 1.0f / static_cast<float>(mips[0].height);
    cb.intensity = enabled ? intensity : 0.0f;
    dc->UpdateSubresource(constantBuffer.Get(), 0, 0, &cb, 0, 0);
    ID3D11ShaderResourceView* srvs[2] = {scene, mips[0].shaderResourceView.Get()};
    dc->PSSetShader(compositeShader.Get(), nullptr, 0);
    dc->PSSetShaderResources(0, 2, srvs);
    dc->Draw(3, 0);

    dc->PSSetShaderResources(0, 2, nullSrvs);
}

void Bloom::DrawGUI()
{
    if (ImGui::Begin("Bloom"))
    {
        ImGui::Checkbox(u8"有効", &enabled);
        ImGui::SliderFloat(u8"しきい値", &threshold, 0.0f, 3.0f);
        ImGui::SliderFloat(u8"しきい値のなめらかさ", &knee, 0.0f, 1.0f);
        ImGui::SliderFloat(u8"強さ", &intensity, 0.0f, 3.0f);
    }

    ImGui::End();
}
