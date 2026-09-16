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

	void Destroy() { destroy_requested = true; }
	bool IsDestroyRequested() const { return destroy_requested; }

protected:
	Enemy() = default;
	bool DamagePlayerOnContact(
		Player& player,
		int damage,
		float invincibleTime,
		bool destroyOnHit = false);

private:
	bool destroy_requested = false;
};
