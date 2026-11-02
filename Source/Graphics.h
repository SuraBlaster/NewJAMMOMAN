#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include "RenderState.h"
#include "ModelRenderer.h"
#include "Bloom.h"
#include "ShapeRenderer.h"
#include "PrimitiveRenderer.h"

// グラフィックス
class Graphics
{
private:
    // コンストラクタ
    Graphics() = default;

    // デストラクタ
    ~Graphics() = default;

public:
    // インスタンス取得
    static Graphics& Instance()
    {
        static Graphics instance;
        return instance;
    }

    // 初期化
    void Initialize(HWND hWnd);

    // クリア
    void Clear(float r, float g, float b, float a);

    // レンダーターゲット設定
    void SetRenderTargets();

    // バックバッファ設定（ブルーム合成後のUI描画用）
    void SetBackBufferTarget();

    // シーンを描いたHDRバッファ取得
    ID3D11ShaderResourceView* GetSceneShaderResourceView() const
    {
        return sceneShaderResourceView.Get();
    }

    // バックバッファ取得
    ID3D11RenderTargetView* GetBackBufferRenderTargetView() const
    {
        return renderTargetView.Get();
    }

    // GUIのパネルに表示するゲーム画面のバッファ取得
    ID3D11RenderTargetView* GetGameViewRenderTargetView() const
    {
        return gameViewRenderTargetView.Get();
    }

    ID3D11ShaderResourceView* GetGameViewShaderResourceView() const
    {
        return gameViewShaderResourceView.Get();
    }

    // ビューポート取得
    const D3D11_VIEWPORT& GetViewport() const
    {
        return viewport;
    }

    // ブルーム取得
    Bloom* GetBloom() const
    {
        return bloom.get();
    }

    // 画面表示
    void Present(UINT syncInterval);

    // ウインドウハンドル取得
    HWND GetWindowHandle()
    {
        return hWnd;
    }

    // デバイス取得
    ID3D11Device* GetDevice()
    {
        return device.Get();
    }

    // デバイスコンテキスト取得
    ID3D11DeviceContext* GetDeviceContext()
    {
        return immediateContext.Get();
    }

    // スクリーン幅取得
    float GetScreenWidth() const
    {
        return screenWidth;
    }

    // スクリーン高さ取得
    float GetScreenHeight() const
    {
        return screenHeight;
    }

    // レンダーステート取得
    RenderState* GetRenderState()
    {
        return renderState.get();
    }

    // プリミティブレンダラ取得
    PrimitiveRenderer* GetPrimitiveRenderer() const
    {
        return primitiveRenderer.get();
    }

    // シェイプレンダラ取得
    ShapeRenderer* GetShapeRenderer() const
    {
        return shapeRenderer.get();
    }

    // モデルレンダラ取得
    ModelRenderer* GetModelRenderer() const
    {
        return modelRenderer.get();
    }

private:
    HWND hWnd = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediateContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> gameViewRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> gameViewShaderResourceView;
    D3D11_VIEWPORT viewport;

    float screenWidth = 0;
    float screenHeight = 0;

    std::unique_ptr<RenderState> renderState;
    std::unique_ptr<PrimitiveRenderer> primitiveRenderer;
    std::unique_ptr<ShapeRenderer> shapeRenderer;
    std::unique_ptr<ModelRenderer> modelRenderer;
    std::unique_ptr<Bloom> bloom;
};
