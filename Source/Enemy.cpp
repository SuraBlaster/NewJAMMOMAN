#include "Enemy.h"

#include "ModelRenderer.h"
#include "ShapeRenderer.h"
#include "Collision.h"
#include "Player.h"

#include <algorithm>
#include <limits>

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

Enemy::CombatEffectPlacement Enemy::GetCombatEffectPlacement() const
{
	// Evaluated on hits only. Use the same transforms as the model renderer.
	using namespace DirectX;
	const float limit = (std::numeric_limits<float>::max)();
	XMFLOAT3 minimum = { limit, limit, limit };
	XMFLOAT3 maximum = { -limit, -limit, -limit };
	bool hasVertices = false;
	if (model)
	{
		for (const auto& mesh : model->GetMeshes())
		{
			if (!mesh.node) continue;
			std::vector<XMFLOAT4X4> boneTransforms;
			for (const auto& bone : mesh.bones)
			{
				XMFLOAT4X4 boneTransform;
				XMStoreFloat4x4(&boneTransform, XMLoadFloat4x4(&bone.offsetTransform)
					* XMLoadFloat4x4(&bone.node->worldTransform));
				boneTransforms.push_back(boneTransform);
			}
			for (const auto& vertex : mesh.vertices)
			{
				const XMVECTOR local = XMLoadFloat3(&vertex.position);
				XMVECTOR world;
				if (boneTransforms.empty())
				{
					world = XMVector3TransformCoord(local, XMLoadFloat4x4(&mesh.node->worldTransform));
				}
				else
				{
					const float weights[] = { vertex.boneWeight.x, vertex.boneWeight.y, vertex.boneWeight.z, vertex.boneWeight.w };
					const unsigned int indices[] = { vertex.boneIndex.x, vertex.boneIndex.y, vertex.boneIndex.z, vertex.boneIndex.w };
					world = XMVectorZero();
					for (int i = 0; i < 4; ++i)
					{
						if (weights[i] <= 0.0f || indices[i] >= boneTransforms.size()) continue;
						world = XMVectorAdd(world, XMVectorScale(XMVector3TransformCoord(
							local, XMLoadFloat4x4(&boneTransforms[indices[i]])), weights[i]));
					}
				}
				XMFLOAT3 point;
				XMStoreFloat3(&point, world);
				minimum = { (std::min)(minimum.x, point.x), (std::min)(minimum.y, point.y), (std::min)(minimum.z, point.z) };
				maximum = { (std::max)(maximum.x, point.x), (std::max)(maximum.y, point.y), (std::max)(maximum.z, point.z) };
				hasVertices = true;
			}
		}
	}
	if (!hasVertices)
		return { { position.x, position.y + height * 0.5f, position.z }, (std::max)(height, radius * 2.0f) };
	return {
		{ (minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f, (minimum.z + maximum.z) * 0.5f },
		(std::max)({ maximum.x - minimum.x, maximum.y - minimum.y, maximum.z - minimum.z, 0.01f })
	};
}
