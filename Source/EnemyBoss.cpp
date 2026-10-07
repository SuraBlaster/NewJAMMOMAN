#include "ModelManager.h"
#include "EnemyBoss.h"
#include "Audio/Audio.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Player.h"
#include "ShapeRenderer.h"

bool EnemyBoss::defeat_flag = false;

EnemyBoss::EnemyBoss(ID3D11Device* device, Player* player)
	: player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Boss.gltf");
	scale = { 0.015f, 0.015f, 0.015f };
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	radius = 0.65f;
	height = 2.0f;
	health = 20;
	use_gravity = true;
	gravity_acceleration = boss_gravity_acceleration;
	color = { 1.0f, 0.35f, 0.2f, 1.0f };
	InitializeAnimator();
	UpdateTransform();
	defeat_flag = false;

	states[static_cast<size_t>(StateId::Idle)] = std::make_unique<IdleState>(this);
	states[static_cast<size_t>(StateId::JumpMove)] = std::make_unique<JumpMoveState>(this);
	states[static_cast<size_t>(StateId::Attack)] = std::make_unique<AttackState>(this);
	states[static_cast<size_t>(StateId::Continuous)] = std::make_unique<ContinuousState>(this);
	states[static_cast<size_t>(StateId::Tornado)] = std::make_unique<TornadoState>(this);
	states[static_cast<size_t>(StateId::Death)] = std::make_unique<DeathState>(this);
	
	SetState(StateId::Idle);
}

void EnemyBoss::SetArenaBounds(float min_x, float max_x)
{
	arena_min_x = (std::min)(min_x, max_x);
	arena_max_x = (std::max)(min_x, max_x);
	// Use the same un-interpolated center as the boss encounter camera.
	arena_center_x = (arena_min_x + arena_max_x) * 0.5f;
	arena_initialized = true;
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
		SetArenaBounds(position.x - default_arena_half_width,
			position.x + default_arena_half_width);
	}

	if (action_enabled || IsDead())
	{
		UpdateStateMachine(elapsedTime);
	}

    if (!IsDead())
    {
        DamagePlayerOnContact(*player, 1, 1.2f);
    }
	UpdateProjectiles(elapsedTime);
	UpdateGroundPhysics(elapsedTime);
	position.x = (std::clamp)(position.x, arena_min_x, arena_max_x);
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}

EnemyBoss::~EnemyBoss()
{
    // Stop tornado sound and visuals even if the encounter is removed mid-attack.
    if (states[static_cast<size_t>(StateId::Tornado)])
        states[static_cast<size_t>(StateId::Tornado)]->OnExit();
    states[static_cast<size_t>(StateId::Death)]->OnExit();
}

void EnemyBoss::UpdateStateMachine(float elapsed_time)
{
    if (next_state != StateId::None)
    {
        if (current_state != StateId::None)
            states[static_cast<size_t>(current_state)]->OnExit();
        current_state = next_state;
        next_state = StateId::None;
        states[static_cast<size_t>(current_state)]->OnEnter();
    }
    if (current_state != StateId::None)
        states[static_cast<size_t>(current_state)]->OnUpdate(elapsed_time);
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
	for (auto& projectile : side_projectiles)
	{
		projectile->Update(elapsedTime);
		projectile->HitPlayer(*player);
	}
	side_projectiles.erase(std::remove_if(side_projectiles.begin(), side_projectiles.end(),
		[](const auto& projectile) { return !projectile->IsActive(); }), side_projectiles.end());
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

void EnemyBoss::OnDead()
{
    // Death can be requested outside Update, so stop the storm immediately.
    states[static_cast<size_t>(StateId::Tornado)]->OnExit();
    side_projectiles.clear();
    projectiles.clear();
    SetState(StateId::Death);
    use_gravity = false;
    velocity = { 0.0f, 0.0f, 0.0f };
    defeat_flag = true;
}
void EnemyBoss::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
	for (const auto& projectile : side_projectiles)
		projectile->DrawDebugPrimitive(shapeRenderer);
	for (const auto& projectile : projectiles) projectile.DrawDebugPrimitive(shapeRenderer);
    const auto* tornado = static_cast<const TornadoState*>(
        states[static_cast<size_t>(StateId::Tornado)].get());
    tornado->DrawDebugPrimitive(shapeRenderer);
}
void EnemyBoss::SetState(StateId state_id)
{
	next_state = state_id;
}

void EnemyBoss::IdleState::OnEnter()
{
	owner->state_timer = owner->GetHealth() <= owner->GetMaxHealth() / 2 ? 0.8f : 1.3f;
	owner->PlayAnimation(0, true);
}

void EnemyBoss::IdleState::OnUpdate(float elapsed_time)
{
	owner->state_timer -= elapsed_time;
	if (owner->state_timer <= 0.0f)
	{
		SelectNextState();

	}
}

void EnemyBoss::IdleState::SelectNextState()
{
	owner->candidates.clear();

	if (owner->lastAction == StateId::Tornado)
	{
		owner->next_state = StateId::Continuous;
	}
	else
	{
		owner->candidates.push_back(StateId::JumpMove);
		owner->candidates.push_back(StateId::Attack);

		if (owner->health <= owner->max_health * 0.5f && owner->lastAction != StateId::Continuous)
		{
			owner->candidates.push_back(StateId::Tornado);
		}

		bool isValid = false;
		int safetyLoop = 0;
		do
		{
			int index = rand() % owner->candidates.size();
			owner->next_state = owner->candidates[index];
			isValid = true;

			if (owner->next_state == StateId::Attack)
			{
				if (owner->lastAction == StateId::Attack && owner->consecutiveAttackCount >= 2) isValid = false;
			}
			else
			{
				if (owner->next_state == owner->lastAction) isValid = false;
			}
			safetyLoop++;
		} while (!isValid && owner->candidates.size() > 1 && safetyLoop < 100);
	}

	owner->consecutiveAttackCount = (owner->next_state == StateId::Attack) ? owner->consecutiveAttackCount + 1 : 0;
	owner->lastAction = owner->next_state;

	owner->SetState(owner->next_state);
}

void EnemyBoss::JumpMoveState::OnEnter()
{
	owner->use_gravity = true;
	owner->gravity_acceleration = boss_gravity_acceleration;
	phase = Phase::Ready;
	jump_count = 0;
	jump_delay = 0.0f;
	if (!owner->arena_initialized)
	{
		owner->SetArenaBounds(owner->position.x - default_arena_half_width,
			owner->position.x + default_arena_half_width);
	}
	move_start_x = owner->position.x;
	owner->velocity = { 0.0f, 0.0f, 0.0f };

	// Start toward the opposite edge; at the center, follow the facing direction.
	const float center_x = owner->arena_center_x;
	const bool move_right = move_start_x < center_x
		|| (move_start_x == center_x && std::sin(owner->rotation.y) > 0.0f);
	// A narrow arena must not produce crossed landing limits.
	const float edge_offset = (std::min)(landing_edge_offset,
		(owner->arena_max_x - owner->arena_min_x) * 0.5f);
	move_target_x = move_right
		? owner->arena_max_x - edge_offset
		: owner->arena_min_x + edge_offset;
	jump_target_x = move_start_x;
}

void EnemyBoss::JumpMoveState::OnUpdate(float elapsed_time)
{
	switch (phase)
	{
	case Phase::Ready:
	{
		// Divide the crossing into three jumps, adapting speed to this room's width.
		jump_target_x = move_start_x + (move_target_x - move_start_x)
			* static_cast<float>(jump_count + 1) / jump_count_limit;
		const float jump_power = jump_powers[jump_count];
		const float flight_time = 2.0f * jump_power / owner->gravity_acceleration;
		owner->velocity.x = (jump_target_x - owner->position.x) / flight_time;
		owner->velocity.y = jump_power;
		owner->is_ground = false;
		owner->PlayAnimation(jump_start_animation, false);
		Audio::Instance().Play(jump_sound);
		phase = Phase::Airborne;
		break;
	}
	case Phase::Airborne:
		if (!owner->IsGround())
		{
			break;
		}

		// Correct the small landing error caused by discrete physics updates.
		owner->position.x = jump_target_x;
		owner->velocity.x = 0.0f;
		++jump_count;
		owner->PlayAnimation(idle_animation, false);
		Audio::Instance().Play(landing_sound);

		if (jump_count >= jump_count_limit)
		{
			phase = Phase::Completed;
			owner->rotation.y += DirectX::XMConvertToRadians(turn_angle_degrees);
			owner->SetState(StateId::Idle);
		}
		else
		{
			jump_delay = owner->GetHealth() <= low_health_threshold
				? low_health_jump_delay : normal_jump_delay;
			phase = Phase::Waiting;
		}
		break;

	case Phase::Waiting:
		jump_delay -= elapsed_time;
		if (jump_delay <= 0.0f)
		{
			phase = Phase::Ready;
		}
		break;

	case Phase::Completed:
		break;
	}
}

void EnemyBoss::AttackState::OnEnter()
{
    phase = Phase::Windup;
    phase_time = 0.0f;
    if (!owner->arena_initialized)
    {
        owner->SetArenaBounds(owner->position.x - default_arena_half_width,
            owner->position.x + default_arena_half_width);
    }
    direction_x = owner->position.x >= owner->arena_center_x ? -1.0f : 1.0f;
    owner->velocity.x = 0.0f;
    owner->PlayAnimation(attack_animation, false);
    Audio::Instance().Play(attack_sound);
}

void EnemyBoss::AttackState::OnUpdate(float elapsed_time)
{
    phase_time += (std::max)(elapsed_time, 0.0f);
    switch (phase)
    {
    case Phase::Windup:
        if (phase_time >= windup_duration)
        {
            phase = Phase::Fire;
            phase_time = 0.0f;
        }
        break;
    case Phase::Fire:
        FireSideProjectiles();
        phase = Phase::Recovery;
        phase_time = 0.0f;
        break;
    case Phase::Recovery:
        if (phase_time >= recovery_duration
            || (owner->animator && !owner->animator->IsPlaying(0)))
        {
            phase = Phase::Completed;
            owner->SetState(StateId::Idle);
        }
        break;
    case Phase::Completed:
        break;
    }
}

void EnemyBoss::AttackState::FireSideProjectiles()
{
    const auto& pattern = patterns[std::rand() % pattern_count];
    const DirectX::XMFLOAT3 start = {
        owner->position.x,
        owner->position.y + owner->height * muzzle_height_ratio,
        owner->position.z
    };
    for (const auto& offset : pattern)
    {
        const DirectX::XMFLOAT3 target = {
            owner->position.x + direction_x
                * (scatter_base_distance + offset.x * scatter_distance_scale),
            owner->position.y + offset.y * scatter_height_scale,
            owner->position.z
        };
        owner->side_projectiles.push_back(
            std::make_unique<BossSideProjectile>(start, target, direction_x));
    }
}
void EnemyBoss::ContinuousState::OnEnter()
{
    time_until_shot = shot_interval;
    volleys_fired = 0;
    if (!owner->arena_initialized)
    {
        owner->SetArenaBounds(owner->position.x - default_arena_half_width,
            owner->position.x + default_arena_half_width);
    }
    direction_x = owner->position.x >= owner->arena_center_x ? -1.0f : 1.0f;
    owner->velocity.x = 0.0f;
    owner->velocity.z = 0.0f;
    owner->PlayAnimation(attack_animation, false);
}

void EnemyBoss::ContinuousState::OnUpdate(float elapsed_time)
{
    if (volleys_fired >= shot_count) return;
    time_until_shot -= (std::max)(elapsed_time, 0.0f);
    if (time_until_shot > 0.0f) return;

    FireVolley();
    ++volleys_fired;
    // Do not emit overlapping catch-up volleys after a slow frame.
    time_until_shot = shot_interval;
    owner->PlayAnimation(attack_animation, false);
    Audio::Instance().Play(attack_sound);
    if (volleys_fired >= shot_count) owner->SetState(StateId::Idle);
}

std::array<DirectX::XMFLOAT3, 5> EnemyBoss::ContinuousState::GenerateSpawnPositions()
{
    std::array<DirectX::XMFLOAT3, spawn_slot_count> positions;
    std::uniform_real_distribution<float> jitter(-spawn_jitter, spawn_jitter);
    for (int index = 0; index < spawn_slot_count; ++index)
    {
        const auto& offset = spawn_offsets[index];
        positions[index] = {
            owner->position.x + direction_x * (offset.x + jitter(random_engine)),
            owner->position.y + offset.y + jitter(random_engine),
            owner->position.z
        };
    }
    std::shuffle(positions.begin(), positions.end(), random_engine);
    return positions;
}

void EnemyBoss::ContinuousState::FireVolley()
{
    const auto positions = GenerateSpawnPositions();
    std::uniform_int_distribution<int> count_distribution(min_projectiles, max_projectiles);
    const int count = count_distribution(random_engine);
    for (int index = 0; index < count; ++index)
    {
        // Spawn at separate positions instead of sharing the boss's muzzle.
        owner->side_projectiles.push_back(std::make_unique<BossSideProjectile>(
            positions[index], positions[index], direction_x, BossSideProjectile::Motion::Continuous));
    }
}
void EnemyBoss::DeathState::OnEnter()
{
    OnExit();
    active = true;
    death_time = 0.0f;
    next_explosion_time = pose_hold_time;
    buildup_started = shock_started = exploded = false;
    base_position = owner->position;
    owner->velocity = { 0.0f, 0.0f, 0.0f };
    owner->use_gravity = false;
    if (owner->animator)
        owner->animator->Play(0, death_animation, false, 0.0f, 0.0f, pose_hold_time);
    Audio::Instance().Stop("BGM_BOSS");
    Audio::Instance().Stop("BGM_STAGE");
    Audio::Instance().Stop("SE_BOSS_STORM");
}

void EnemyBoss::DeathState::PlayVisual(Visual visual, const DirectX::XMFLOAT3& position, float scale)
{
    const size_t index = static_cast<size_t>(visual);
    if (!effects[index]) effects[index] = std::make_unique<Effect>(effect_paths[index]);
    const auto handle = effects[index]->Play(position, scale);
    if (handle >= 0) effect_handles.emplace_back(visual, handle);
}

void EnemyBoss::DeathState::StopVisuals()
{
    for (const auto& entry : effect_handles)
        effects[static_cast<size_t>(entry.first)]->Stop(entry.second);
    effect_handles.clear();
}

void EnemyBoss::DeathState::OnUpdate(float elapsed_time)
{
    if (!active) return;
    death_time += (std::max)(elapsed_time, 0.0f);
    if (death_time < explosion_time)
    {
        const float random_x = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        const float random_y = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        owner->position = { base_position.x + random_x * shake_power,
            base_position.y + random_y * shake_power, base_position.z };
        if (death_time >= pose_hold_time && !buildup_started)
        {
            buildup_started = true;
            PlayVisual(Visual::Light, { base_position.x, base_position.y + 0.75f, base_position.z }, 0.3f);
            PlayVisual(Visual::Smoke, { base_position.x, base_position.y, base_position.z - 0.5f }, 3.0f);
            PlayVisual(Visual::Flare, { base_position.x, base_position.y, base_position.z + 0.5f }, 7.0f);
            PlayVisual(Visual::BlackExplosion, { base_position.x, base_position.y, base_position.z - 0.3f }, 1.5f);
            Audio::Instance().Play("SE_BOSS_DEATH");
        }
        if (death_time >= next_explosion_time)
        {
            next_explosion_time = death_time + small_explosion_interval;
            PlayVisual(Visual::SmallExplosion,
                { base_position.x + random_x * small_explosion_width * 2.0f,
                  base_position.y + (random_y + 0.5f) * owner->height, base_position.z - 0.1f });
        }
        if (death_time >= shock_time && !shock_started)
        {
            shock_started = true;
            PlayVisual(Visual::Shock, base_position);
        }
    }
    if (death_time >= explosion_time && !exploded)
    {
        exploded = true;
        StopVisuals();
        owner->position = base_position;
        const DirectX::XMFLOAT3 center = { base_position.x,
            base_position.y + owner->height * 0.6f, base_position.z };
        PlayVisual(Visual::Explosion, center, 0.5f);
        PlayVisual(Visual::Fire, center);
        Audio::Instance().Stop("SE_BOSS_DEATH");
        Audio::Instance().Play("SE_DEATH_GENERIC");
        owner->scale = { 0.0f, 0.0f, 0.0f };
    }
    if (death_time >= death_duration)
    {
        OnExit();
        owner->Destroy();
    }
}

void EnemyBoss::DeathState::OnExit()
{
    StopVisuals();
    if (active) Audio::Instance().Stop("SE_BOSS_DEATH");
    active = false;
}
void EnemyBoss::TornadoState::OnEnter()
{
    OnExit();
    if (!owner->arena_initialized)
    {
        owner->SetArenaBounds(owner->position.x - default_arena_half_width,
            owner->position.x + default_arena_half_width);
    }
    // Follow the room's center and the boss's ground height, not the camera blend.
    tornado_position = { owner->arena_center_x,
        owner->position.y + base_y_offset, owner->position.z };
    remaining_time = owner->GetHealth() <= low_health_threshold
        ? low_health_duration : normal_duration;
    owner->velocity.x = 0.0f;
    owner->velocity.z = 0.0f;
    if (!wind_effect)
        wind_effect = std::make_unique<Effect>("Data/Effect/wind.efkefc");
    wind_handle = wind_effect->Play(tornado_position);
    active = true;
    owner->PlayAnimation(attack_animation, false);
    Audio::Instance().Play(attack_sound);
    Audio::Instance().Play(storm_sound, true);
}

void EnemyBoss::TornadoState::OnUpdate(float elapsed_time)
{
    if (!active) return;
    if (owner->IsDead() || !owner->player)
    {
        OnExit();
        return;
    }
    const float step = (std::min)((std::max)(elapsed_time, 0.0f), remaining_time);
    if (step > 0.0f && !owner->player->IsDead())
    {
        UpdateBacksidePush(step);
        UpdatePullAndCollision(step);
    }
    remaining_time -= step;
    if (remaining_time <= 0.0f)
    {
        OnExit();
        owner->SetState(StateId::Idle);
    }
}

void EnemyBoss::TornadoState::OnExit()
{
    if (wind_effect && wind_handle >= 0)
        wind_effect->Stop(wind_handle);
    wind_handle = -1;
    if (active) Audio::Instance().Stop(storm_sound);
    active = false;
    remaining_time = 0.0f;
}

float EnemyBoss::TornadoState::GetRadiusAtHeight(float relative_y) const
{
    const float height_ratio = (std::clamp)(relative_y / tornado_height, 0.0f, 1.0f);
    return bottom_radius + (top_radius - bottom_radius) * height_ratio;
}

void EnemyBoss::TornadoState::UpdatePullAndCollision(float elapsed_time)
{
    Player& player = *owner->player;
    auto player_position = player.GetPosition();
    const float relative_y = player_position.y - tornado_position.y;
    if (relative_y < -vertical_margin || relative_y > tornado_height + vertical_margin)
        return;

    const float radius_at_height = GetRadiusAtHeight(relative_y);
    float dx = tornado_position.x - player_position.x;
    const float dz = tornado_position.z - player_position.z;
    float distance = std::sqrt(dx * dx + dz * dz);
    if (distance < radius_at_height * pull_range_multiplier && distance > pull_min_distance)
    {
        // Preserve 2D movement and prevent crossing the center on a long frame.
        const float move_x = dx / distance * pull_speed * elapsed_time;
        player_position.x += (std::clamp)(move_x, -std::abs(dx), std::abs(dx));
        player.SetPosition(player_position);
        dx = tornado_position.x - player_position.x;
        distance = std::sqrt(dx * dx + dz * dz);
    }
    if (distance < radius_at_height + player.GetRadius())
    {
        if (player.GetVelocity().y < lift_speed_threshold)
            player.AddImpulse({ 0.0f, lift_impulse, 0.0f });
        if (player.GetInvincibleTime() <= 0.0f)
            player.ApplyDamage(damage, invincible_duration);
    }
}

void EnemyBoss::TornadoState::UpdateBacksidePush(float elapsed_time)
{
    Player& player = *owner->player;
    const auto& player_position = player.GetPosition();
    if (std::abs(player_position.y - owner->position.y) > backside_height_tolerance)
        return;

    const float direction = owner->position.x >= owner->arena_center_x ? -1.0f : 1.0f;
    const float dx = player_position.x - owner->position.x;
    if (dx * direction >= backside_front_margin) return;

    const bool is_near = std::abs(dx) < backside_near_distance;
    const float acceleration = is_near ? backside_near_acceleration : backside_push_acceleration;
    const float max_speed = is_near ? backside_near_max_speed : backside_max_speed;
    const float forward_speed = player.GetVelocity().x * direction;
    if (forward_speed >= max_speed) return;

    const float impulse = (std::min)(acceleration * elapsed_time, max_speed - forward_speed);
    player.AddImpulse({ direction * impulse,
        is_near ? backside_lift_acceleration * elapsed_time : 0.0f, 0.0f });
}

void EnemyBoss::TornadoState::DrawDebugPrimitive(ShapeRenderer* renderer) const
{
    if (!active || !renderer) return;
    // Cross-sections use the same height/radius function as collision detection.
    for (int index = 0; index <= debug_segments; ++index)
    {
        const float y = tornado_height * static_cast<float>(index) / debug_segments;
        renderer->DrawSphere({ tornado_position.x, tornado_position.y + y, tornado_position.z },
            GetRadiusAtHeight(y), { 0.1f, 0.8f, 1.0f, 1.0f });
    }
}
