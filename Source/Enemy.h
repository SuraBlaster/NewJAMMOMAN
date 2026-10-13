#pragma once

#include "Character.h"

class ModelRenderer;
class ShapeRenderer;
class Player;

class Enemy : public Character
{
public:
	~Enemy() override = default;

	virtual void Render(ModelRenderer* modelRenderer) const;
	virtual void DrawPrimitive(ShapeRenderer*) const {}
	virtual void DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const;

	// Visual body placement, independent of the gameplay collider.
	struct CombatEffectPlacement
	{
		DirectX::XMFLOAT3 position;
		float size;
	};
	virtual CombatEffectPlacement GetCombatEffectPlacement() const;

	void Destroy() { destroyRequested = true; }
	bool IsDestroyRequested() const { return destroyRequested; }

	static constexpr float DAMAGE_INVINCIBLE_DURATION = 0.05f;
	bool ApplyDamage(int damage, float invincibleDuration = DAMAGE_INVINCIBLE_DURATION)
	{
		return Character::ApplyDamage(damage, invincibleDuration);
	}

protected:
	Enemy() = default;
	bool DamagePlayerOnContact(
		Player& player,
		int damage,
		float invincibleTime,
		bool destroyOnHit = false);

private:
	bool destroyRequested = false;
};
