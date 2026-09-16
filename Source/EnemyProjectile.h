#pragma once

#include <DirectXMath.h>

class Player;
class ShapeRenderer;

class EnemyProjectile
{
public:
	EnemyProjectile(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& velocity,
		float radius = 0.18f,
		float lifetime = 5.0f,
		int damage = 1);

	void Update(float elapsedTime);
	bool HitPlayer(Player& player, float invincibleTime);
	void DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const;

	bool IsActive() const { return active; }
	void Destroy() { active = false; }

private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	float radius;
	float lifetime;
	int damage;
	bool active = true;
};
