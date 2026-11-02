#pragma once

#include "Shader.h"

// 輪郭線（背面法）シェーダー
class OutlineShader : public Shader
{
public:
    OutlineShader(ID3D11Device* device);
    ~OutlineShader() override = default;

    // 開始処理
    void Begin(const RenderContext& rc) override;

    // 更新処理
    void Update(const RenderContext& rc, const Model::Mesh& mesh) override;

    // 終了処理
    void End(const RenderContext& rc) override;

private:
    // Shader/Outline.hlsli の CbOutline と同じ並びにすること
    struct CbOutline
    {
        DirectX::XMFLOAT4 color;
        float width;
        float albedoMix;
        DirectX::XMFLOAT2 viewportSize;
    };

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> outlineConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> cullFrontRasterizerState;
};
