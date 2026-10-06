#include "Collision.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	float Dot(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	DirectX::XMFLOAT3 Subtract(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}

	DirectX::XMFLOAT3 AddScaled(
		const DirectX::XMFLOAT3& value,
		const DirectX::XMFLOAT3& direction,
		float scale)
	{
		return {
			value.x + direction.x * scale,
			value.y + direction.y * scale,
			value.z + direction.z * scale
		};
	}

	float ClosestPointsOnSegments(
		const DirectX::XMFLOAT3& p1,
		const DirectX::XMFLOAT3& q1,
		const DirectX::XMFLOAT3& p2,
		const DirectX::XMFLOAT3& q2,
		DirectX::XMFLOAT3& closest1,
		DirectX::XMFLOAT3& closest2)
	{
		constexpr float epsilon = 1.0e-6f;
		const DirectX::XMFLOAT3 d1 = Subtract(q1, p1);
		const DirectX::XMFLOAT3 d2 = Subtract(q2, p2);
		const DirectX::XMFLOAT3 r = Subtract(p1, p2);
		const float a = Dot(d1, d1);
		const float e = Dot(d2, d2);
		const float f = Dot(d2, r);
		float s = 0.0f;
		float t = 0.0f;

		if (a <= epsilon && e <= epsilon)
		{
			closest1 = p1;
			closest2 = p2;
			return Dot(r, r);
		}
		if (a <= epsilon)
		{
			t = (std::clamp)(f / e, 0.0f, 1.0f);
		}
		else
		{
			const float c = Dot(d1, r);
			if (e <= epsilon)
			{
				s = (std::clamp)(-c / a, 0.0f, 1.0f);
			}
			else
			{
				const float b = Dot(d1, d2);
				const float denominator = a * e - b * b;
				if (denominator != 0.0f)
					s = (std::clamp)((b * f - c * e) / denominator, 0.0f, 1.0f);
				t = (b * s + f) / e;
				if (t < 0.0f)
				{
					t = 0.0f;
					s = (std::clamp)(-c / a, 0.0f, 1.0f);
				}
				else if (t > 1.0f)
				{
					t = 1.0f;
					s = (std::clamp)((b - c) / a, 0.0f, 1.0f);
				}
			}
		}

		closest1 = AddScaled(p1, d1, s);
		closest2 = AddScaled(p2, d2, t);
		const DirectX::XMFLOAT3 difference = Subtract(closest1, closest2);
		return Dot(difference, difference);
	}
}

bool Collision::IntersectSphereVsSphere(
	const DirectX::XMFLOAT3& positionA, float radiusA,
	const DirectX::XMFLOAT3& positionB, float radiusB,
	DirectX::XMFLOAT3& outPositionB)
{
	const DirectX::XMFLOAT3 difference = Subtract(positionB, positionA);
	const float distanceSquared = Dot(difference, difference);
	const float radius = radiusA + radiusB;
	if (distanceSquared > radius * radius) return false;

	const float distance = std::sqrt(distanceSquared);
	if (distance > 1.0e-6f)
		outPositionB = AddScaled(positionA, difference, radius / distance);
	else
		outPositionB = { positionA.x + radius, positionA.y, positionA.z };
	return true;
}

bool Collision::IntersectCylinderVsCylinder(
	const DirectX::XMFLOAT3& positionA, float radiusA, float heightA,
	const DirectX::XMFLOAT3& positionB, float radiusB, float heightB,
	DirectX::XMFLOAT3& outPositionB)
{
	if (positionA.y > positionB.y + heightB || positionA.y + heightA < positionB.y)
		return false;

	const float dx = positionB.x - positionA.x;
	const float dz = positionB.z - positionA.z;
	const float distanceSquared = dx * dx + dz * dz;
	const float radius = radiusA + radiusB;
	if (distanceSquared > radius * radius) return false;

	const float distance = std::sqrt(distanceSquared);
	if (distance > 1.0e-6f)
		outPositionB = { positionA.x + dx / distance * radius, positionB.y, positionA.z + dz / distance * radius };
	else
		outPositionB = { positionA.x + radius, positionB.y, positionA.z };
	return true;
}

bool Collision::IntersectSphereVsCylinder(
	const DirectX::XMFLOAT3& spherePosition, float sphereRadius,
	const DirectX::XMFLOAT3& cylinderPosition, float cylinderRadius, float cylinderHeight,
	DirectX::XMFLOAT3& outCylinderPosition)
{
	const float dx = spherePosition.x - cylinderPosition.x;
	const float dz = spherePosition.z - cylinderPosition.z;
	const float horizontalLength = std::sqrt(dx * dx + dz * dz);
	const float nearestY = (std::clamp)(
		spherePosition.y,
		cylinderPosition.y,
		cylinderPosition.y + cylinderHeight);
	const float horizontalGap = (std::max)(0.0f, horizontalLength - cylinderRadius);
	const float verticalGap = spherePosition.y - nearestY;
	if (horizontalGap * horizontalGap + verticalGap * verticalGap > sphereRadius * sphereRadius)
		return false;

	if (horizontalLength > 1.0e-6f)
	{
		const float nearestRadius = (std::min)(horizontalLength, cylinderRadius);
		outCylinderPosition = {
			cylinderPosition.x + dx / horizontalLength * nearestRadius,
			nearestY,
			cylinderPosition.z + dz / horizontalLength * nearestRadius
		};
	}
	else
	{
		outCylinderPosition = { cylinderPosition.x, nearestY, cylinderPosition.z };
	}
	return true;
}

bool Collision::IntersectCapsuleVsCylinder(
	const DirectX::XMFLOAT3& capsuleStart,
	const DirectX::XMFLOAT3& capsuleEnd,
	float capsuleRadius,
	const DirectX::XMFLOAT3& cylinderPosition,
	float cylinderRadius,
	float cylinderHeight,
	DirectX::XMFLOAT3& outContactPosition)
{
	const DirectX::XMFLOAT3 cylinderEnd = {
		cylinderPosition.x,
		cylinderPosition.y + cylinderHeight,
		cylinderPosition.z
	};
	DirectX::XMFLOAT3 closestBlade;
	DirectX::XMFLOAT3 closestCylinder;
	const float distanceSquared = ClosestPointsOnSegments(
		capsuleStart, capsuleEnd,
		cylinderPosition, cylinderEnd,
		closestBlade, closestCylinder);
	const float radius = capsuleRadius + cylinderRadius;
	if (distanceSquared > radius * radius) return false;

	outContactPosition = {
		(closestBlade.x + closestCylinder.x) * 0.5f,
		(closestBlade.y + closestCylinder.y) * 0.5f,
		(closestBlade.z + closestCylinder.z) * 0.5f
	};
	return true;
}

bool Collision::IsHitAABBToAABB(
	const DirectX::XMFLOAT3& aMin, const DirectX::XMFLOAT3& aMax,
	const DirectX::XMFLOAT3& bMin, const DirectX::XMFLOAT3& bMax)
{
	return aMin.x <= bMax.x && aMax.x >= bMin.x
		&& aMin.y <= bMax.y && aMax.y >= bMin.y
		&& aMin.z <= bMax.z && aMax.z >= bMin.z;
}

bool Collision::PushBackFromMultipleAABB(
	const DirectX::XMFLOAT3& playerMin,
	const DirectX::XMFLOAT3& playerMax,
	const std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>& blocks,
	DirectX::XMFLOAT3& totalOffset)
{
	totalOffset = { 0, 0, 0 };
	DirectX::XMFLOAT3 currentMin = playerMin;
	DirectX::XMFLOAT3 currentMax = playerMax;
	bool collided = false;
	for (int iteration = 0; iteration < 10; ++iteration)
	{
		bool pushed = false;
		for (const auto& block : blocks)
		{
			DirectX::XMFLOAT3 offset;
			if (!PushBackAABB(currentMin, currentMax, block.first, block.second, offset)) continue;
			currentMin.x += offset.x; currentMin.y += offset.y; currentMin.z += offset.z;
			currentMax.x += offset.x; currentMax.y += offset.y; currentMax.z += offset.z;
			totalOffset.x += offset.x; totalOffset.y += offset.y; totalOffset.z += offset.z;
			collided = true;
			pushed = true;
		}
		if (!pushed) break;
	}
	return collided;
}

bool Collision::PushBackAABB(
	const DirectX::XMFLOAT3& minA, const DirectX::XMFLOAT3& maxA,
	const DirectX::XMFLOAT3& minB, const DirectX::XMFLOAT3& maxB,
	DirectX::XMFLOAT3& outOffset)
{
	if (!IsHitAABBToAABB(minA, maxA, minB, maxB)) return false;
	const float candidates[6] = {
		minB.x - maxA.x, maxB.x - minA.x,
		minB.y - maxA.y, maxB.y - minA.y,
		minB.z - maxA.z, maxB.z - minA.z
	};
	int best = 0;
	for (int i = 1; i < 6; ++i)
	{
		if (std::abs(candidates[i]) < std::abs(candidates[best])) best = i;
	}
	outOffset = { 0, 0, 0 };
	if (best < 2) outOffset.x = candidates[best];
	else if (best < 4) outOffset.y = candidates[best];
	else outOffset.z = candidates[best];
	return true;
}

void Collision::TransformAABB(
	const DirectX::XMFLOAT4X4& transform,
	const DirectX::XMFLOAT3& localMin,
	const DirectX::XMFLOAT3& localMax,
	DirectX::XMFLOAT3& outWorldMin,
	DirectX::XMFLOAT3& outWorldMax)
{
	const DirectX::XMFLOAT3 corners[8] = {
		{ localMin.x, localMin.y, localMin.z }, { localMax.x, localMin.y, localMin.z },
		{ localMin.x, localMax.y, localMin.z }, { localMax.x, localMax.y, localMin.z },
		{ localMin.x, localMin.y, localMax.z }, { localMax.x, localMin.y, localMax.z },
		{ localMin.x, localMax.y, localMax.z }, { localMax.x, localMax.y, localMax.z }
	};
	const DirectX::XMMATRIX matrix = DirectX::XMLoadFloat4x4(&transform);
	outWorldMin = {
		(std::numeric_limits<float>::max)(),
		(std::numeric_limits<float>::max)(),
		(std::numeric_limits<float>::max)()
	};
	outWorldMax = {
		(std::numeric_limits<float>::lowest)(),
		(std::numeric_limits<float>::lowest)(),
		(std::numeric_limits<float>::lowest)()
	};
	for (const auto& corner : corners)
	{
		DirectX::XMFLOAT3 world;
		DirectX::XMStoreFloat3(&world,
			DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&corner), matrix));
		outWorldMin.x = (std::min)(outWorldMin.x, world.x);
		outWorldMin.y = (std::min)(outWorldMin.y, world.y);
		outWorldMin.z = (std::min)(outWorldMin.z, world.z);
		outWorldMax.x = (std::max)(outWorldMax.x, world.x);
		outWorldMax.y = (std::max)(outWorldMax.y, world.y);
		outWorldMax.z = (std::max)(outWorldMax.z, world.z);
	}
}

SweepStatus Collision::SweepAABB(const AABB& movingBox, const DirectX::XMFLOAT3& displacement, const AABB& obstacle, SweepHit& hit)
{
	hit = SweepHit{};

	// 1. 開始時点で、体積を持って重なっているか。
	// 面が触れているだけなら、初期めり込みにはしない。
	const bool overlapping =
		movingBox.min.x < obstacle.max.x &&
		movingBox.max.x > obstacle.min.x &&
		movingBox.min.y < obstacle.max.y &&
		movingBox.max.y > obstacle.min.y &&
		movingBox.min.z < obstacle.max.z &&
		movingBox.max.z > obstacle.min.z;

	if (overlapping)
	{
		return SweepStatus::InitialOverlap;
	}

	// 2. 障害物をプレイヤーの半サイズ分だけ膨らませる。
	const DirectX::XMFLOAT3 center = movingBox.GetCenter();
	const DirectX::XMFLOAT3 half = movingBox.GetHalfSize();

	const float p[3] = {
		center.x, center.y, center.z
	};

	const float d[3] = {
		displacement.x, displacement.y, displacement.z
	};

	const float expandedMin[3] = {
		obstacle.min.x - half.x,
		obstacle.min.y - half.y,
		obstacle.min.z - half.z
	};

	const float expandedMax[3] = {
		obstacle.max.x + half.x,
		obstacle.max.y + half.y,
		obstacle.max.z + half.z
	};

	const float infinity =
		std::numeric_limits<float>::infinity();

	float axisEnter[3] = {
		-infinity, -infinity, -infinity
	};

	float enterTime = -infinity;
	float exitTime = infinity;

	// 3. 各軸の進入時刻・退出時刻を求める。
	for (int axis = 0; axis < 3; ++axis)
	{
		if (d[axis] == 0.0f)
		{
			// この軸では移動しない。
			// 範囲外、または面に沿うだけなら進入しない。
			if (p[axis] <= expandedMin[axis] ||
				p[axis] >= expandedMax[axis])
			{
				return SweepStatus::NoHit;
			}

			continue;
		}

		const float t1 =
			(expandedMin[axis] - p[axis]) / d[axis];

		const float t2 =
			(expandedMax[axis] - p[axis]) / d[axis];

		const float nearTime = (std::min)(t1, t2);
		const float farTime = (std::max)(t1, t2);

		axisEnter[axis] = nearTime;

		enterTime = (std::max)(enterTime, nearTime);
		exitTime = (std::min)(exitTime, farTime);
	}

	// 4. 今回の移動区間で、箱の内部へ進入するか。
	if (enterTime < 0.0f ||
		enterTime > 1.0f ||
		exitTime <= 0.0f ||
		enterTime >= exitTime)
	{
		return SweepStatus::NoHit;
	}

	hit.time = enterTime;

	// 5. 最後に進入した軸が、接触面になる。
	// ほぼ同時なら複数の面として保持する。
	constexpr float timeTolerance = 0.000001f;

	for (int axis = 0; axis < 3; ++axis)
	{
		if (d[axis] == 0.0f)
		{
			continue;
		}

		if (std::abs(axisEnter[axis] - enterTime)
	> timeTolerance)
		{
			continue;
		}

		DirectX::XMFLOAT3 normal = {
			0.0f, 0.0f, 0.0f
		};

		const float sign =
			d[axis] > 0.0f ? -1.0f : 1.0f;

		if (axis == 0)
		{
			normal.x = sign;
		}
		else if (axis == 1)
		{
			normal.y = sign;
		}
		else
		{
			normal.z = sign;
		}

		hit.normals[hit.normalCount++] = normal;
	}

	return SweepStatus::Hit;
}
