#pragma once

#include <DirectXMath.h>
#include <memory>

class Player;
class ShapeRenderer;
class Effect;

class EnemyProjectile
{
  public:

    // コンストラクタ
    EnemyProjectile(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity,
                    float radius = 0.18f, float lifetime = 5.0f, int damage = 1);

    // デストラクタ
    ~EnemyProjectile();

    // コンストラクタ
    EnemyProjectile(EnemyProjectile&& other) noexcept;
    EnemyProjectile& operator=(EnemyProjectile&& other) noexcept;

    // コンストラクタ
    EnemyProjectile(const EnemyProjectile&) = delete;
    EnemyProjectile& operator=(const EnemyProjectile&) = delete;

    void AttachEffect(const std::shared_ptr<Effect>& effect, float scale);

    // 更新処理
    void Update(float elapsedTime);
    bool HitPlayer(Player& player, float invincibleTime);

    // 当たり判定などのデバッグ描画処理
    void DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const;

    bool IsActive() const
    {
        return active;
    }

    void Destroy();

  private:

    struct VisualEffect;
    std::unique_ptr<VisualEffect> visualEffect;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    float radius;
    float lifetime;
    int damage;
    bool active = true;
};
