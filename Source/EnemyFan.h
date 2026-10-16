#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyFan final : public Enemy
{
  public:

    // コンストラクタ
    EnemyFan(ID3D11Device* device, Player* player);

    // 更新処理
    void Update(float elapsedTime) override;

  private:

    void OnDead() override
    {
        Destroy();
    }

    Player* player = nullptr;
};
