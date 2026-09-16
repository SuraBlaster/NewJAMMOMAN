#pragma once

#include "Graphics/Model.h"
#include "Enemy.h"
#include "Projectile/ProjectileManager.h"

//ボスエネミー
class LoadingBoss : public Enemy
{
public:
    LoadingBoss();
    ~LoadingBoss() override;

    // 更新処理
    void Update(float elapsedTime)override;

    // 描画処理
    void Render(ModelRenderer* modelRenderer)override;

private:

    // ステート遷移
    void TransitionIdleState();
    void TransitionAttackState();

    // ステート更新処理
    void UpdateIdleState(float elapsedTime);
    void UpdateAttackState(float elapsedTime);
private:
    // ステート
    enum class State
    {
        Idle,       // 待機
        Attack,     // 攻撃モーション
    };

    //アニメーション
    enum Animation
    {
        Anim_Idle,
        Anim_Attack,
        Anim_JumpStart,
        Anim_JumpApex,
        Anim_JumpEnd,
        Anim_Jump,
        Anim_Demo,
    };
private:
    State state = State::Idle;
    std::vector<Model::NodePose> nodePoses;

    // タイマー
    float attackTimer = 1.0f;

    Effekseer::Handle bossDemoHandle = -1;

   std::shared_ptr<Effect> bossDemoEffect;

   PointLight pl{};
   int lightIndex;
};
