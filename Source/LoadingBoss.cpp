#include "LoadingBoss.h"
#include "Graphics/Graphics.h"
#include "Mathf.h"
#include "Player.h"
#include "Collision.h"
#include "Projectile/ProjectileSideL.h"
#include "Projectile/ProjectileSideR.h"
#include <cstdlib>
#include <ctime>
#include <Scene/SceneManager.h>
#include <Scene/SceneLoading.h>
#include <Scene/SceneClear.h>

LoadingBoss::LoadingBoss()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    model = std::make_shared<Model>(device, "Data/Model/Enemy/Boss.gltf");

    // スケーリング
    scale.x = scale.y = scale.z = 0.015f;

    position.y = 15.0f;
    position.z = -9.0f;

    angle.y = DirectX::XMConvertToRadians(180);

    // ノードポーズ取得
    model->GetNodePoses(nodePoses);

    bossDemoEffect = std::make_shared<Effect>("Data/Effect/DemoBoss.efkefc");

    lightIndex = LightManager::Instance().AllocatePointLight();

    // 待機ステートへ遷移
    TransitionIdleState();
}

LoadingBoss::~LoadingBoss()
{
}

void LoadingBoss::Update(float elapsedTime)
{
    // ステートごとの更新処理
    switch (state)
    {
    case State::Idle:
        UpdateIdleState(elapsedTime);
        break;
    case State::Attack:
        UpdateAttackState(elapsedTime);
        break;
    }

    // オブジェクト行列を更新
    UpdateTransform();

    // 速力処理更新
    UpdateVelocity(elapsedTime);

    // 無敵時間更新
    UpdateInvincibleTimer(elapsedTime);

    // モデルアニメーション更新
    model->UpdateAnimation(elapsedTime, nodePoses);
    model->SetNodePoses(nodePoses);

    // モデル行列を更新
    model->UpdateTransform(transform);

    if (position.y < 10.0f)
    {
        position.y = 10.0f;
    }
}

void LoadingBoss::Render(ModelRenderer* modelRenderer)
{
    modelRenderer->Draw(ShaderId::Toon, model);
}

void LoadingBoss::TransitionIdleState()
{
    state = State::Idle;

    // 待機アニメーション再生
    model->PlayAnimation(Anim_Idle, true);
}

void LoadingBoss::UpdateIdleState(float elapsedTime)
{
    attackTimer -= elapsedTime;

    if (attackTimer <= 0.0f)
    {
        TransitionAttackState();
    }
}
void LoadingBoss::TransitionAttackState()
{
    state = State::Attack;

    model->PlayAnimation(Anim_Attack, false);

    attackTimer = 0.5f;

    Model::Node* node = model->FindNode("hand_l");
    if (!node) return;

    DirectX::XMFLOAT3 nodePos(
        node->worldTransform._41,
        node->worldTransform._42,
        node->worldTransform._43
    );

    /*if (lightIndex != -1)
    {
        pl.position = { nodePos.x,nodePos.y,nodePos.z, 1.0f };
        pl.color = { 1.0f, 0.0f, 0.0f, 1.0f };
        pl.range = 2.0f;

        LightManager::Instance().SetPointLight(pl, lightIndex);
    }*/


}

void LoadingBoss::UpdateAttackState(float elapsedTime)
{
}