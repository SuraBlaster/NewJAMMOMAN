#pragma once

#include "Enemy.h"
#include "EnemyActivation.h"

class Player;

class EnemyWalker final : public Enemy
{
  public:

    // コンストラクタ
    EnemyWalker(ID3D11Device* device, Player* player);

    // 更新処理
    void Update(float elapsedTime) override;
    void SetDirection(float direction);

  private:

    // UpdatePatrol の更新処理
    void UpdatePatrol(float elapsedTime);
    void OnDead() override;

    Player* player = nullptr;
    EnemyActivation activation;

    float moveDirection = -1.0f;
    float moveSpeed = 1.0f;

    bool placementInitialized = false;
    bool walkingAnimationStarted = false;
};
