#pragma once

#include <d3d11.h>
#include <vector>

#include "Enemy.h"
#include "EnemyProjectile.h"

class Player;

class EnemyMage final : public Enemy
{
  public:

    // コンストラクタ
    EnemyMage(ID3D11Device* device, Player* player);

    // 更新処理
    void Update(float elapsedTime) override;
    void SetDirection(float direction);

    // DrawPrimitive の描画処理
    void DrawPrimitive(ShapeRenderer* shapeRenderer) const override;

  private:

    void FireProjectile();

    // UpdateProjectiles の更新処理
    void UpdateProjectiles(float elapsedTime);

    void OnDead() override
    {
        Destroy();
    }

    Player* player = nullptr;
    std::vector<EnemyProjectile> projectiles;
    std::shared_ptr<Effect> projectileEffect;
    float attackTimer = 1.5f;
    float facingDirection = -1.0f;
};
