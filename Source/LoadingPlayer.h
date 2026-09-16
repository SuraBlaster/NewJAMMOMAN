#pragma once

#include "Graphics/Shader.h"
#include "Graphics/Model.h"
#include "Character.h"
#include "Projectile/ProjectileManager.h"
#include "Effect/Effect.h"

class LoadingPlayer : public Character
{
public:
    LoadingPlayer();
    ~LoadingPlayer()override;

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
    };
private:

    std::vector<Model::NodePose>nodePoses;

    //Effekseer
};
