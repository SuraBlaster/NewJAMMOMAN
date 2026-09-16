#pragma once

#include "Graphics/Shader.h"
#include "Graphics/Model.h"
#include "Character.h"
#include "Projectile/ProjectileManager.h"
#include "Effect/Effect.h"

class ClearPlayer : public Character
{
private:

    //待機ステートへ遷移
    void TransitionIdleState();

    //待機ステート更新処理
    void UpdateIdleState(float elapsedTime);

    //移動ステートへ遷移
    void TransitionMoveState();

    //移動ステート更新処理
    void UpdateMoveState(float elapsedTime);

    //手を振るステートへ遷移
    void TransitionWaveState();

    //手を振るステート更新処理
    void UpdateWaveState(float elapsedTime);
public:
    ClearPlayer();
    ~ClearPlayer()override;

    //インスタンス取得
    static ClearPlayer& Instance();

    //更新処理
    void Update(float elapsedTime);

    //描画処理
    void Render(ModelRenderer* modelRenderer);

private:
    //アニメーション
    enum Animation
    {
        Anim_Idle,
        Anim_Run,
        Anim_Dash,
        Anim_Jump,
        Anim_Land,
        Anim_Shot,
        Anim_Damage,
        Anim_Wave,
    };

    enum class State
    {
        Idle,
        Move,
        Wave,
    };
private:

    float moveSpeed = 5.0f;

    State state = State::Idle;

    float timer = 3.0f;

    bool flag = false;

    std::vector<Model::NodePose>nodePoses;
};
