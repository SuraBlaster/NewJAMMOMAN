#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Animator.h"
#include "Model.h"

class Character
{
  public:

    // コンストラクタ
    Character() = default;

    // デストラクタ
    virtual ~Character() = default;

    // コンストラクタ
    Character(const Character&) = delete;
    Character& operator=(const Character&) = delete;

    // 更新処理
    virtual void Update(float elapsedTime) = 0;

    void SetPosition(const DirectX::XMFLOAT3& value)
    {
        position = value;
    }

    const DirectX::XMFLOAT3& GetPosition() const
    {
        return position;
    }

    void SetRotation(const DirectX::XMFLOAT3& value)
    {
        rotation = value;
    }

    const DirectX::XMFLOAT3& GetRotation() const
    {
        return rotation;
    }

    void SetScale(const DirectX::XMFLOAT3& value)
    {
        scale = value;
    }

    const DirectX::XMFLOAT3& GetScale() const
    {
        return scale;
    }

    void SetColor(const DirectX::XMFLOAT4& value)
    {
        color = value;
    }

    const DirectX::XMFLOAT4& GetColor() const
    {
        return color;
    }

    void SetVelocity(const DirectX::XMFLOAT3& value)
    {
        velocity = value;
    }

    const DirectX::XMFLOAT3& GetVelocity() const
    {
        return velocity;
    }

    void AddImpulse(const DirectX::XMFLOAT3& impulse);

    bool IsGround() const
    {
        return isGround;
    }

    float GetRadius() const
    {
        return radius;
    }

    float GetHeight() const
    {
        return height;
    }

    int GetHealth() const
    {
        return health;
    }

    int GetMaxHealth() const
    {
        return maxHealth;
    }

    bool IsDead() const
    {
        return health <= 0;
    }

    float GetInvincibleTime() const
    {
        return invincibleTime;
    }

    bool ApplyDamage(int damage, float invincibleDuration = 0.0f);

    std::shared_ptr<Model> GetModel() const
    {
        return model;
    }

    const DirectX::XMFLOAT4X4& GetTransform() const
    {
        return transform;
    }

  protected:

    // 変換行列の更新処理
    void UpdateTransform();

    // 無敵時間の更新処理
    void UpdateInvincibleTimer(float elapsedTime);

    // UpdateGroundPhysics の更新処理
    void UpdateGroundPhysics(float elapsedTime);

    // InitializeAnimator の初期化処理
    void InitializeAnimator(int initialClip = 0, bool loop = true);
    void PlayAnimation(int clip, bool loop, float blendDuration = 0.1f);
    void SetHealth(int value);

    virtual void OnLanding()
    {
    }

    virtual void OnDamaged()
    {
    }

    virtual void OnDead()
    {
    }

  protected:

    std::shared_ptr<Model> model;
    std::unique_ptr<Animator> animator;
    DirectX::XMFLOAT3 position = {0, 0, 0};
    DirectX::XMFLOAT3 rotation = {0, 0, 0};
    DirectX::XMFLOAT3 scale = {1, 1, 1};
    DirectX::XMFLOAT4X4 transform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    DirectX::XMFLOAT4 color = {1, 1, 1, 1};
    DirectX::XMFLOAT3 velocity = {0, 0, 0};
    float radius = 0.4f;
    float height = 1.0f;
    bool isGround = false;
    bool groundPlacementChecked = false;
    int health = 5;
    int maxHealth = 5;
    float invincibleTime = 0.0f;
    float gravityAcceleration = 10.0f;
    bool useGravity = false;
};
