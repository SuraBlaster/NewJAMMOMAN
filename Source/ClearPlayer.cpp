#include "ClearPlayer.h"
#include "ModelManager.h"
#include "Graphics.h"
#include "ModelRenderer.h"
#include <algorithm>

ClearPlayer::ClearPlayer()
{
    model = ModelManager::Instance().CreateInstance(Graphics::Instance().GetDevice(),
        "Data/Model/Jammo/Jammo_Player.gltf");
    position = {16.0f, -6.5f, 0};
    rotation = {0, DirectX::XMConvertToRadians(-90), 0};
    scale = {0.018f, 0.018f, 0.018f};
    InitializeAnimator(model->GetAnimationIndex("Idle_Seq_0"));
    Update(0);
}

void ClearPlayer::BeginEntrance()
{
    if (state != State::Waiting) return;
    state = State::Move;
    PlayAnimation(model->GetAnimationIndex("Run_Fast_Loop_Seq_0"), true, 0.2f);
    if (animator) animator->SetLayerSpeed(0, 1.5f);
}

void ClearPlayer::Update(float elapsedTime)
{
    const float dt = (std::max)(0.0f, elapsedTime);
    if (state == State::Move)
    {
        position.x = (std::max)(0.0f, position.x - MoveSpeed * dt);
        if (position.x <= 0)
        {
            state = State::Wave;
            rotation.y = DirectX::XMConvertToRadians(180);
            PlayAnimation(model->GetAnimationIndex("Wave"), true, 0.3f);
            if (animator) animator->SetLayerSpeed(0, 1.0f);
        }
    }
    if (animator) animator->Update(dt);
    UpdateTransform();
}

void ClearPlayer::Render(ModelRenderer* renderer)
{
    if (state != State::Waiting) renderer->Draw(ShaderId::Model, model);
}
