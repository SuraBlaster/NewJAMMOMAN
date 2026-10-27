#include "Misc.h"
#include "GpuResourceUtils.h"
#include "OutlineShader.h"
#include "StylizedShading.h"

OutlineShader::OutlineShader(ID3D11Device* device)
{
    // 頂点シェーダー
    GpuResourceUtils::LoadVertexShader(device, "Data/Shader/OutlineVS.cso", Model::INPUT_ELEMENT_DESCS.data(),
                                       static_cast<UINT>(Model::INPUT_ELEMENT_DESCS.size()),
                                       inputLayout.GetAddressOf(), vertexShader.GetAddressOf());

    // ピクセルシェーダー
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/OutlinePS.cso", pixelShader.GetAddressOf());

    // 輪郭線用定数バッファ
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbOutline), outlineConstantBuffer.GetAddressOf());

    // 表面カリング（裏面だけを描く）
    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FrontCounterClockwise = false;
        desc.DepthClipEnable = true;
        desc.MultisampleEnable = true;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_FRONT;
        HRESULT hr = device->CreateRasterizerState(&desc, cullFrontRasterizerState.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }
}

// 開始処理
void OutlineShader::Begin(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // 線の太さをピクセル指定にするため、現在の画面サイズを取得する
    D3D11_VIEWPORT viewport{};
    UINT viewportCount = 1;
    dc->RSGetViewports(&viewportCount, &viewport);

    // 輪郭線用定数バッファ更新
    const StylizedShading::OutlineSettings& settings = StylizedShading::Instance().GetOutlineSettings();
    CbOutline cbOutline{};
    cbOutline.color = {settings.color.x, settings.color.y, settings.color.z, 1.0f};
    cbOutline.width = settings.width;
    cbOutline.albedoMix = settings.albedoMix;
    cbOutline.viewportSize = {viewport.Width, viewport.Height};
    dc->UpdateSubresource(outlineConstantBuffer.Get(), 0, 0, &cbOutline, 0, 0);

    // シェーダー設定
    dc->IASetInputLayout(inputLayout.Get());
    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);

    // 定数バッファ設定
    ID3D11Buffer* cbs[] = {outlineConstantBuffer.Get()};
    dc->VSSetConstantBuffers(1, _countof(cbs), cbs);
    dc->PSSetConstantBuffers(1, _countof(cbs), cbs);

    // 裏面だけを描く
    dc->RSSetState(cullFrontRasterizerState.Get());
}

// 更新処理
void OutlineShader::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // 線の色にモデルの色を混ぜるため、ベーステクスチャを設定する
    ID3D11ShaderResourceView* srvs[] = {mesh.material->baseMap.Get()};
    dc->PSSetShaderResources(0, _countof(srvs), srvs);
}

// 終了処理
void OutlineShader::End(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // シェーダー設定解除
    dc->VSSetShader(nullptr, nullptr, 0);
    dc->PSSetShader(nullptr, nullptr, 0);
    dc->IASetInputLayout(nullptr);

    // 定数バッファ設定解除
    ID3D11Buffer* cbs[] = {nullptr};
    dc->VSSetConstantBuffers(1, _countof(cbs), cbs);
    dc->PSSetConstantBuffers(1, _countof(cbs), cbs);

    // シェーダーリソースビュー設定解除
    ID3D11ShaderResourceView* srvs[] = {nullptr};
    dc->PSSetShaderResources(0, _countof(srvs), srvs);

    // ModelRendererの通常設定（裏面カリング）に戻す
    dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));
}
