#pragma once

#include <d3d11.h>
#include "Enemy.h"
#include "EnemyActivation.h"

class Player;

class EnemyFly final : public Enemy
{
  public:

    // コンストラクタ
    EnemyFly(ID3D11Device* device, Player* player);

    // 更新処理
    void Update(float elapsedTime) override;

  private:

    void OnDead() override
    {
        Destroy();
    }

    ID3D11Device* device = nullptr;
    Player* player = nullptr;
    float moveSpeed = 5.0f;
    bool eggDropped = false;
    EnemyActivation activation;
};
