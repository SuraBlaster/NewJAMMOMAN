#pragma once

#include "Character.h"

class ModelRenderer;
class ShapeRenderer;
class Player;

class Enemy : public Character
{
  public:

    // デストラクタ
    ~Enemy() override = default;

    // 描画処理
    virtual void Render(ModelRenderer* modelRenderer) const;

    // DrawPrimitive の描画処理
    virtual void DrawPrimitive(ShapeRenderer*) const
    {
    }

    // 当たり判定などのデバッグ描画処理
    virtual void DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const;

    struct CombatEffectPlacement
    {
        DirectX::XMFLOAT3 position;
        float size;
    };

    virtual CombatEffectPlacement GetCombatEffectPlacement() const;

    void Destroy()
    {
        destroyRequested = true;
    }

    bool IsDestroyRequested() const
    {
        return destroyRequested;
    }

    static constexpr float DAMAGE_INVINCIBLE_DURATION = 0.05f;

    bool ApplyDamage(int damage, float invincibleDuration = DAMAGE_INVINCIBLE_DURATION)
    {
        return Character::ApplyDamage(damage, invincibleDuration);
    }

  protected:

    // コンストラクタ
    Enemy() = default;
    bool DamagePlayerOnContact(Player& player, int damage, float invincibleTime, bool destroyOnHit = false);

  private:

    bool destroyRequested = false;
};
