#pragma once

#include "Effect/Effect.h"

class Player;
class ShapeRenderer;

class BossSideProjectile final
{
  public:

    enum class Motion
    {
        ScatterAndAccelerate,
        Continuous
    };

    // コンストラクタ
    BossSideProjectile(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& scatterTarget,
                       float directionX, Motion motion = Motion::ScatterAndAccelerate);

    // デストラクタ
    ~BossSideProjectile();

    // コンストラクタ
    BossSideProjectile(const BossSideProjectile&) = delete;
    BossSideProjectile& operator=(const BossSideProjectile&) = delete;

    // 更新処理
    void Update(float elapsedTime);
    void HitPlayer(Player& player);

    // 当たり判定などのデバッグ描画処理
    void DrawDebugPrimitive(ShapeRenderer* renderer) const;

    bool IsActive() const
    {
        return active;
    }

    static constexpr float GetRadius()
    {
        return RADIUS;
    }

  private:

    enum class Phase
    {
        Scattering,
        Moving
    };
    void Destroy();

    static constexpr float SCATTER_SPEED = 20.0f;
    static constexpr float SCATTER_DURATION = 1.0f;
    static constexpr float CONTINUOUS_START_DELAY = 0.1f;
    static constexpr float ACCELERATION = 10.0f;
    static constexpr float MAX_SPEED = 15.0f;
    static constexpr float LIFETIME = 4.0f;
    static constexpr float RADIUS = 0.4f;
    static constexpr int DAMAGE = 1;
    static constexpr float INVINCIBLE_DURATION = 1.2f;
    static constexpr float KNOCKBACK_POWER = 5.0f;
    static constexpr float EFFECT_SCALE = 0.08f;
    static constexpr float EFFECT_Y_OFFSET = -0.5f;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scatterTarget;
    float directionX;
    Motion motion;
    float age = 0.0f;
    float currentSpeed = 0.0f;
    Phase phase = Phase::Scattering;
    bool active = true;
    Effect windEffect{"Data/Effect/wind.efkefc"};
    Effekseer::Handle windHandle = -1;
};
