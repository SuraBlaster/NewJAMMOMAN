#include "ModelManager.h"
#include "LoadingPlayer.h"
#include "Graphics.h"
#include "ModelRenderer.h"
#include "GpuResourceUtils.h"
#include <algorithm>
#include <stdexcept>

// コンストラクタ
LoadingPlayer::LoadingPlayer()
{
    auto* device = Graphics::Instance().GetDevice();
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Jammo/Jammo_Player.gltf");
    position = {6.1f, -6.75f, 0};
    rotation = {0, DirectX::XMConvertToRadians(-90), 0};
    scale = {0.012f, 0.012f, 0.012f};

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> white;

    if (FAILED(GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, white.GetAddressOf())))
    {
        throw std::runtime_error("Failed to create loading player texture");
    }

    for (const auto& mesh : model->GetMeshes())
    {
        mesh.material->baseMap = white;
        mesh.material->baseColor = {1, 1, 1, 1};
        mesh.material->alphaMode = Model::AlphaMode::Opaque;
    }

    InitializeAnimator(model->GetAnimationIndex("Run_Fast_Loop_Seq_0"));

    if (animator)
    {
        animator->SetLayerSpeed(0, 1.5f);
    }

    Update(0);
}

// 更新処理
void LoadingPlayer::Update(float elapsedTime)
{
    if (animator)
    {
        animator->Update((std::max)(0.0f, elapsedTime));
    }

    UpdateTransform();
}

// 描画処理
void LoadingPlayer::Render(ModelRenderer* renderer)
{
    renderer->Draw(ShaderId::Basic, model);
}
