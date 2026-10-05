#include "Graphics.h"
#include "EffectManager.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

void EffectManager::Initialize()
{
    if (effekseerManager.Get() != nullptr) return;
    auto& graphics = Graphics::Instance();
    if (!graphics.GetDevice() || !graphics.GetDeviceContext())
        throw std::runtime_error("Initialize Graphics before EffectManager");
    effekseerRenderer = EffekseerRendererDX11::Renderer::Create(graphics.GetDevice(), graphics.GetDeviceContext(), 2048);
    effekseerManager = Effekseer::Manager::Create(2048);
    if (effekseerRenderer.Get() == nullptr || effekseerManager.Get() == nullptr)
    {
        Finalize();
        throw std::runtime_error("Effekseer initialization failed");
    }
    effekseerRenderer->SetRestorationOfStatesFlag(true);
    effekseerManager->SetSpriteRenderer(effekseerRenderer->CreateSpriteRenderer());
    effekseerManager->SetRibbonRenderer(effekseerRenderer->CreateRibbonRenderer());
    effekseerManager->SetRingRenderer(effekseerRenderer->CreateRingRenderer());
    effekseerManager->SetTrackRenderer(effekseerRenderer->CreateTrackRenderer());
    effekseerManager->SetModelRenderer(effekseerRenderer->CreateModelRenderer());
    effekseerManager->SetTextureLoader(effekseerRenderer->CreateTextureLoader());
    effekseerManager->SetModelLoader(effekseerRenderer->CreateModelLoader());
    effekseerManager->SetMaterialLoader(effekseerRenderer->CreateMaterialLoader());
    effekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
    time = 0.0f;
}

void EffectManager::Finalize()
{
    if (effekseerManager.Get() != nullptr) effekseerManager->StopAllEffects();
    effekseerManager.Reset();
    effekseerRenderer.Reset();
    time = 0.0f;
}

void EffectManager::Update(float elapsedTime)
{
    if (effekseerManager.Get() == nullptr || !std::isfinite(elapsedTime) || elapsedTime < 0.0f) return;
    time += elapsedTime;
    effekseerRenderer->SetTime(time);
    effekseerManager->Update(elapsedTime * 60.0f);
}

void EffectManager::Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
    if (effekseerManager.Get() == nullptr || effekseerRenderer.Get() == nullptr) return;
    Effekseer::Matrix44 cameraMatrix, projectionMatrix;
    static_assert(sizeof(cameraMatrix) == sizeof(view), "Matrix size mismatch");
    std::memcpy(&cameraMatrix, &view, sizeof(view));
    std::memcpy(&projectionMatrix, &projection, sizeof(projection));
    effekseerRenderer->SetCameraMatrix(cameraMatrix);
    effekseerRenderer->SetProjectionMatrix(projectionMatrix);
    effekseerRenderer->ResetRenderState();
    if (effekseerRenderer->BeginRendering())
    {
        effekseerManager->Draw();
        effekseerRenderer->EndRendering();
    }
}

