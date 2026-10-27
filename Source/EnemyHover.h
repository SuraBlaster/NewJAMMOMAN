#pragma once

#include <d3d11.h>
#include <vector>

#include "Enemy.h"
#include "EnemyProjectile.h"

class Player;

class EnemyHover final : public Enemy
{
  public:

    // コンストラクタ
    EnemyHover(ID3D11Device* device, Player* player);

    // 更新処理
    void Update(float elapsedTime) override;

    // DrawPrimitive の描画処理
    void DrawPrimitive(ShapeRenderer* shapeRenderer) const override;

  private:

    void FireRadialProjectiles();

    // UpdateProjectiles の更新処理
    void UpdateProjectiles(float elapsedTime);

    void OnDead() override
    {
        Destroy();
    }

    Player* player = nullptr;
    std::vector<EnemyProjectile> projectiles;
    std::shared_ptr<Effect> projectileEffect;
    std::shared_ptr<Effect> terrainImpactEffect;
    float attackTimer = 2.0f;
    float hoverTime = 0.0f;
    float baseY = 0.0f;
};
