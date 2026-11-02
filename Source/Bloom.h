#pragma once

#include <wrl.h>
#include <d3d11.h>

// 明るい部分を縮小と拡大を繰り返してぼかし、光のにじみとしてシーンに足す
class Bloom
{
public:
    Bloom(ID3D11Device* device, UINT width, UINT height);

    // sceneをブルーム付きでtargetへ書き出す
    void Render(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* scene, ID3D11RenderTargetView* target,
                const D3D11_VIEWPORT& viewport);

    // 調整用GUI
    void DrawGUI();

private:
    static constexpr int MIP_COUNT = 6;

    struct CbBloom
    {
        float texelSize[2];
        float threshold;
        float knee;
        float intensity;
        float padding[3];
    };

    struct MipTarget
    {
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        UINT width;
        UINT height;
    };

    void DrawFullscreen(ID3D11DeviceContext* dc, ID3D11PixelShader* pixelShader,
                        ID3D11ShaderResourceView* source, UINT sourceWidth, UINT sourceHeight,
                        ID3D11RenderTargetView* target, UINT targetWidth, UINT targetHeight);

    bool enabled = true;
    float threshold = 1.3f; // 白いキャラが光らないよう、剣などのより明るい部分だけを光らせる
    float knee = 0.1f; // しきい値付近をなめらかにつなぐ幅
    float intensity = 0.65f;

    UINT sceneWidth = 0;
    UINT sceneHeight = 0;
    MipTarget mips[MIP_COUNT];

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> prefilterShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> downsampleShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> upsampleShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> compositeShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> linearClampSampler;
    Microsoft::WRL::ComPtr<ID3D11BlendState> opaqueBlendState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> additiveBlendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> noDepthState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
};
