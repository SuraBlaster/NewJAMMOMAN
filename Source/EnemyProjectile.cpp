#include "EnemyProjectile.h"

#include "Collision.h"
#include "Player.h"
#include "ShapeRenderer.h"

EnemyProjectile::EnemyProjectile(
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& velocity,
	float radius,
	float lifetime,
	int damage)
	: position(position)
	, velocity(velocity)
	, radius(radius)
	, lifetime(lifetime)
	, damage(damage)
{
}

void EnemyProjectile::Update(float elapsedTime)
{
	if (!active) return;
	position.x += velocity.x * elapsedTime;
	position.y += velocity.y * elapsedTime;
	position.z += velocity.z * elapsedTime;
	lifetime -= elapsedTime;
	if (lifetime <= 0.0f) active = false;
}

bool EnemyProjectile::HitPlayer(Player& player, float invincibleTime)
{
	if (!active) return false;
	DirectX::XMFLOAT3 contact;
	if (!Collision::IntersectSphereVsCylinder(
		position, radius,
		player.GetPosition(), player.GetRadius(), player.GetHeight(),
		contact))
	{
		return false;
	}
	player.ApplyDamage(damage, invincibleTime);
	active = false;
	return true;
}

void EnemyProjectile::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
	if (active && shapeRenderer)
	{
		shapeRenderer->DrawSphere(position, radius, { 1.0f, 0.2f, 0.8f, 1.0f });
	}
}
