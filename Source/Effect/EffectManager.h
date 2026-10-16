#pragma once
#include <DirectXMath.h>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

class EffectManager
{
  public:

    static EffectManager& Instance()
    {
        static EffectManager instance;
        return instance;
    }

    // コンストラクタ
    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;

    // 初期化処理
    void Initialize();

    // 終了処理
    void Finalize();

    // 更新処理
    void Update(float elapsedTime);

    // 描画処理
    void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

    Effekseer::ManagerRef GetEffekseerManager()
    {
        return effekseerManager;
    }

  private:

    // コンストラクタ
    EffectManager() = default;

    // デストラクタ
    ~EffectManager() = default;
    Effekseer::ManagerRef effekseerManager;
    EffekseerRenderer::RendererRef effekseerRenderer;
    float time = 0.0f;
};
