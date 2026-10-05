#include "ModelManager.h"
#include "LoadingBoss.h"
#include "Graphics.h"
#include "ModelRenderer.h"
#include <algorithm>

LoadingBoss::LoadingBoss()
{
    model = ModelManager::Instance().CreateInstance(Graphics::Instance().GetDevice(), "Data/Model/Enemy/Boss.gltf");
    position = {8.0f, -4.7f, 0};
    rotation = {0, DirectX::XMConvertToRadians(180), 0};
    scale = {0.06f, 0.06f, 0.06f};
    InitializeAnimator(model->GetAnimationIndex("Anim_ZMIKE_Idle"));
    Update(0);
}

void LoadingBoss::Update(float elapsedTime)
{
    const float dt = (std::max)(0.0f, elapsedTime);
    attackTimer -= dt;
    if (!attacking && attackTimer <= 0)
    {
        attacking = true;
        PlayAnimation(model->GetAnimationIndex("Anim_ZMIKE_FireL"), false);
    }
    if (animator) animator->Update(dt);
    UpdateTransform();
}

void LoadingBoss::Render(ModelRenderer* renderer)
{
    renderer->Draw(ShaderId::Model, model);
}
