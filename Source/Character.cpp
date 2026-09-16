#include "Character.h"

#include <algorithm>
#include "CollisionManager.h"

void Character::UpdateTransform()
{
	const DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	const DirectX::XMMATRIX rotationMatrix =
		DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	const DirectX::XMMATRIX translation =
		DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMStoreFloat4x4(&transform, scaling * rotationMatrix * translation);
	if (model)
	{
		model->UpdateTransform(transform);
	}
}

bool Character::ApplyDamage(int damage, float invincibleTime)
{
	if (damage <= 0 || IsDead() || invincible_time > 0.0f)
	{
		return false;
	}
	health = (std::max)(0, health - damage);
	invincible_time = (std::max)(0.0f, invincibleTime);
	if (IsDead()) OnDead();
	else OnDamaged();
	return true;
}

void Character::AddImpulse(const DirectX::XMFLOAT3& impulse)
{
	velocity.x += impulse.x;
	velocity.y += impulse.y;
	velocity.z += impulse.z;
}

void Character::UpdateInvincibleTimer(float elapsedTime)
{
	invincible_time = (std::max)(0.0f, invincible_time - elapsedTime);
}

void Character::UpdateGroundPhysics(float elapsedTime)
{
	position.x += velocity.x * elapsedTime;
	position.z += velocity.z * elapsedTime;

	if (!use_gravity)
	{
		position.y += velocity.y * elapsedTime;
		return;
	}

	velocity.y -= gravity_acceleration * elapsedTime;
	const float moveY = velocity.y * elapsedTime;
	if (moveY >= 0.0f)
	{
		position.y += moveY;
		is_ground = false;
		return;
	}

	const DirectX::XMFLOAT3 start = { position.x, position.y + height, position.z };
	const DirectX::XMFLOAT3 end = { position.x, position.y + moveY, position.z };
	HitResult hit;
	if (CollisionManager::Instance().Raycast(start, end, hit))
	{
		const bool landedThisFrame = !is_ground;
		position.y = hit.position.y;
		velocity.y = 0.0f;
		velocity.x = 0.0f;
		velocity.z = 0.0f;
		is_ground = true;
		if (landedThisFrame) OnLanding();
	}
	else
	{
		position.y += moveY;
		is_ground = false;
	}
}

void Character::InitializeAnimator(int initialClip, bool loop)
{
	if (!model || model->GetAnimations().empty()) return;
	animator = std::make_unique<Animator>(model.get());
	PlayAnimation(initialClip, loop, 0.0f);
}

void Character::PlayAnimation(int clip, bool loop, float blendDuration)
{
	if (!animator || !model || clip < 0
		|| static_cast<size_t>(clip) >= model->GetAnimations().size()) return;
	animator->Play(0, clip, loop, blendDuration);
}

void Character::SetHealth(int value)
{
	max_health = (std::max)(1, value);
	health = max_health;
}
