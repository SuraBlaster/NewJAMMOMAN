#include "EnemyBoss.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Player.h"
#include "ShapeRenderer.h"

bool EnemyBoss::defeat_flag = false;

EnemyBoss::EnemyBoss(ID3D11Device* device, Player* player)
	: player(player)
{
	model = std::make_shared<Model>(device, "Data/Model/Enemy/Boss.gltf");
	scale = { 0.015f, 0.015f, 0.015f };
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	radius = 0.65f;
	height = 2.0f;
	SetHealth(20);
	use_gravity = true;
	color = { 1.0f, 0.35f, 0.2f, 1.0f };
	InitializeAnimator();
	UpdateTransform();
	defeat_flag = false;
}

void EnemyBoss::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}
	if (!arena_initialized)
	{
		arena_center_x = position.x;
		arena_initialized = true;
	}

	if (state == State::Death)
	{
		state_timer -= elapsedTime;
		if (state_timer <= 0.0f) Destroy();
	}
	else if (action_enabled)
	{
		switch (state)
		{
		case State::Idle:
			state_timer -= elapsedTime;
			if (state_timer <= 0.0f)
			{
				state = next_state;
				state_timer = next_state == State::Tornado ? 3.0f : 2.0f;
				shot_timer = 0.0f;
				shots_remaining = next_state == State::Continuous ? 5 : 1;
				jump_started = false;
				PlayAnimation(1, false);
			}
			break;

		case State::JumpMove:
			if (!jump_started)
			{
				const float direction = player->GetPosition().x < position.x ? -1.0f : 1.0f;
				velocity = { direction * 6.0f, 10.0f, 0.0f };
				jump_started = true;
			}
			else if (is_ground)
			{
				EnterIdle();
			}
			break;

		case State::Attack:
			if (shots_remaining > 0)
			{
				for (int i = -2; i <= 2; ++i) FireAtPlayer(i * 0.12f);
				shots_remaining = 0;
			}
			state_timer -= elapsedTime;
			if (state_timer <= 0.0f) EnterIdle();
			break;

		case State::Continuous:
			shot_timer -= elapsedTime;
			if (shots_remaining > 0 && shot_timer <= 0.0f)
			{
				FireAtPlayer();
				--shots_remaining;
				shot_timer = 0.35f;
			}
			if (shots_remaining <= 0) EnterIdle();
			break;

		case State::Tornado:
			UpdateTornado(elapsedTime);
			state_timer -= elapsedTime;
			if (state_timer <= 0.0f) EnterIdle();
			break;

		case State::Death:
			break;
		}
	}

	if (state != State::Death)
		DamagePlayerOnContact(*player, 1, 1.2f);

	UpdateProjectiles(elapsedTime);
	UpdateGroundPhysics(elapsedTime);
	position.x = (std::clamp)(position.x, arena_center_x - 7.0f, arena_center_x + 7.0f);
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}

void EnemyBoss::EnterIdle()
{
	state = State::Idle;
	state_timer = GetHealth() <= GetMaxHealth() / 2 ? 0.8f : 1.3f;
	PlayAnimation(0, true);
	SelectNextAction();
}

void EnemyBoss::SelectNextAction()
{
	const int optionCount = GetHealth() <= GetMaxHealth() / 2 ? 4 : 3;
	switch (std::rand() % optionCount)
	{
	case 0: next_state = State::JumpMove; break;
	case 1: next_state = State::Attack; break;
	case 2: next_state = State::Continuous; break;
	default: next_state = State::Tornado; break;
	}
}

void EnemyBoss::FireAtPlayer(float angleOffset)
{
	const DirectX::XMFLOAT3 start = { position.x, position.y + height * 0.65f, position.z };
	DirectX::XMFLOAT3 direction = {
		player->GetPosition().x - start.x,
		player->GetPosition().y + player->GetHeight() * 0.5f - start.y,
		player->GetPosition().z - start.z
	};
	DirectX::XMVECTOR vector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&direction));
	DirectX::XMStoreFloat3(&direction, vector);
	const float cosine = std::cos(angleOffset);
	const float sine = std::sin(angleOffset);
	const float x = direction.x * cosine - direction.y * sine;
	const float y = direction.x * sine + direction.y * cosine;
	projectiles.emplace_back(
		start,
		DirectX::XMFLOAT3{ x * 7.0f, y * 7.0f, direction.z * 7.0f },
		0.25f,
		6.0f,
		1);
}

void EnemyBoss::UpdateProjectiles(float elapsedTime)
{
	for (auto& projectile : projectiles)
	{
		projectile.Update(elapsedTime);
		projectile.HitPlayer(*player, 1.2f);
	}
	projectiles.erase(
		std::remove_if(projectiles.begin(), projectiles.end(),
			[](const EnemyProjectile& projectile) { return !projectile.IsActive(); }),
		projectiles.end());
}

void EnemyBoss::UpdateTornado(float elapsedTime)
{
	DirectX::XMFLOAT3 playerPosition = player->GetPosition();
	const float dx = position.x - playerPosition.x;
	const float distance = std::abs(dx);
	if (distance < 8.0f && distance > 0.05f)
	{
		playerPosition.x += dx / distance * 4.0f * elapsedTime;
		player->SetPosition(playerPosition);
	}
	if (distance < 1.5f)
	{
		player->ApplyDamage(1, 1.2f);
		player->AddImpulse({ 0.0f, 3.0f * elapsedTime, 0.0f });
	}
}

void EnemyBoss::OnDead()
{
	state = State::Death;
	state_timer = 2.0f;
	use_gravity = false;
	velocity = { 0, 0, 0 };
	defeat_flag = true;
	PlayAnimation(6, false);
}

void EnemyBoss::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
	for (const auto& projectile : projectiles) projectile.DrawDebugPrimitive(shapeRenderer);
	if (state == State::Tornado && shapeRenderer)
	{
		for (int i = 0; i <= 8; ++i)
		{
			const float t = i / 8.0f;
			shapeRenderer->DrawSphere(
				{ position.x, position.y + t * 6.0f, position.z },
				0.5f + t * 2.0f,
				{ 0.1f, 0.8f, 1.0f, 1.0f });
		}
	}
}
