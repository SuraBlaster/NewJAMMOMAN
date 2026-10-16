#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyEgg final : public Enemy
{
  public:

    // コンストラクタ
    EnemyEgg(ID3D11Device* device, Player* player, const DirectX::XMFLOAT3& spawnPosition);

    // 更新処理
    void Update(float elapsedTime) override;

  private:

    void OnLanding() override;

    void OnDead() override
    {
        Destroy();
    }

    ID3D11Device* device = nullptr;
    Player* player = nullptr;
};
