#pragma once

#include <DirectXMath.h>
#include <utility>
#include <vector>

// Primitive collision helpers that do not depend on rendering resources.
class Collision
{
public:
	static bool IntersectSphereVsSphere(
		const DirectX::XMFLOAT3& positionA, float radiusA,
		const DirectX::XMFLOAT3& positionB, float radiusB,
		DirectX::XMFLOAT3& outPositionB);

	static bool IntersectCylinderVsCylinder(
		const DirectX::XMFLOAT3& positionA, float radiusA, float heightA,
		const DirectX::XMFLOAT3& positionB, float radiusB, float heightB,
		DirectX::XMFLOAT3& outPositionB);

	static bool IntersectSphereVsCylinder(
		const DirectX::XMFLOAT3& spherePosition, float sphereRadius,
		const DirectX::XMFLOAT3& cylinderPosition, float cylinderRadius, float cylinderHeight,
		DirectX::XMFLOAT3& outCylinderPosition);

	// The capsule is an arbitrary segment with thickness. The cylinder is vertical.
	static bool IntersectCapsuleVsCylinder(
		const DirectX::XMFLOAT3& capsuleStart,
		const DirectX::XMFLOAT3& capsuleEnd,
		float capsuleRadius,
		const DirectX::XMFLOAT3& cylinderPosition,
		float cylinderRadius,
		float cylinderHeight,
		DirectX::XMFLOAT3& outContactPosition);

	static bool IsHitAABBToAABB(
		const DirectX::XMFLOAT3& aMin, const DirectX::XMFLOAT3& aMax,
		const DirectX::XMFLOAT3& bMin, const DirectX::XMFLOAT3& bMax);

	static bool PushBackFromMultipleAABB(
		const DirectX::XMFLOAT3& playerMin,
		const DirectX::XMFLOAT3& playerMax,
		const std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>& blocks,
		DirectX::XMFLOAT3& totalOffset);

	static bool PushBackAABB(
		const DirectX::XMFLOAT3& minA, const DirectX::XMFLOAT3& maxA,
		const DirectX::XMFLOAT3& minB, const DirectX::XMFLOAT3& maxB,
		DirectX::XMFLOAT3& outOffset);

	static void TransformAABB(
		const DirectX::XMFLOAT4X4& transform,
		const DirectX::XMFLOAT3& localMin,
		const DirectX::XMFLOAT3& localMax,
		DirectX::XMFLOAT3& outWorldMin,
		DirectX::XMFLOAT3& outWorldMax);
};
