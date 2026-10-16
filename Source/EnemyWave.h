#pragma once

#include <d3d11.h>

#include "Enemy.h"
#include "EnemyActivation.h"

class Player;

class EnemyWave final : public Enemy
{
  public:

    enum class Direction
    {
        Left,
        Right
    };

    // コンストラクタ
    EnemyWave(ID3D11Device* device, Player* player);

    // デストラクタ
    ~EnemyWave() override = default;

    // 更新処理
    void Update(float elapsedTime) override;

    void SetDirection(Direction value);

    void SetWave(float speed, float amplitude)
    {
        waveSpeed = speed;
        waveAmplitude = amplitude;
    }

  private:

    // UpdateAttacking の更新処理
    void UpdateAttacking(float elapsedTime);
    void CollideWithPlayer();
    bool IsOutOfRange() const;

    void OnDead() override
    {
        Destroy();
    }

    Player* player = nullptr;
    Direction direction = Direction::Right;
    float targetY = 0.0f;
    EnemyActivation activation;
    bool placementInitialized = false;
    float moveSpeed = 1.5f;
    float waveSpeed = 4.0f;
    float waveAmplitude = 0.5f;
    float waveTime = 0.0f;
};
