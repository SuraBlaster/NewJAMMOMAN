#include "Enemy.h"

#include "ModelRenderer.h"
#include "ShapeRenderer.h"
#include "Collision.h"
#include "Player.h"

void Enemy::Render(ModelRenderer* modelRenderer) const
{
	if (modelRenderer && model)
	{
		modelRenderer->Draw(ShaderId::Model, model, color, color.w);
	}
}

void Enemy::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
	if (shapeRenderer)
	{
		shapeRenderer->DrawCapsule(transform, radius, height, { 1, 0, 0, 1 });
	}
}

bool Enemy::DamagePlayerOnContact(
	Player& player,
	int damage,
	float invincibleTime,
	bool destroyOnHit)
{
	DirectX::XMFLOAT3 correctedPosition;
	if (!Collision::IntersectCylinderVsCylinder(
		position, radius, height,
		player.GetPosition(), player.GetRadius(), player.GetHeight(),
		correctedPosition))
	{
		return false;
	}

	const bool damaged = player.ApplyDamage(damage, invincibleTime);
	if (destroyOnHit) Destroy();
	return damaged;
}
