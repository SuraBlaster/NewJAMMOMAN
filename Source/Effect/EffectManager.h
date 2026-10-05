#pragma once
#include <DirectXMath.h>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

// Use on the main/render thread, like Graphics and SceneManager.
class EffectManager
{
public:
    static EffectManager& Instance() { static EffectManager instance; return instance; }
    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;
    void Initialize();
    void Finalize();
    void Update(float elapsedTime);
    void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);
    Effekseer::ManagerRef GetEffekseerManager() { return effekseerManager; }
private:
    EffectManager() = default;
    ~EffectManager() = default;
    Effekseer::ManagerRef effekseerManager;
    EffekseerRenderer::RendererRef effekseerRenderer;
    float time = 0.0f;
};
