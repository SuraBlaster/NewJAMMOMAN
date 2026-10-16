#pragma once

#include <DirectXMath.h>
#include <utility>
#include <vector>

struct AABB
{

  public:

    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;

    DirectX::XMFLOAT3 GetCenter() const
    {
        return {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f};
    }

    DirectX::XMFLOAT3 GetHalfSize() const
    {
        return {(max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f, (max.z - min.z) * 0.5f};
    }
};

enum class SweepStatus
{
    NoHit,         // 移動経路に衝突なし
    Hit,           // 移動中に接触する
    InitialOverlap // 移動前から重なっている
};

struct SweepHit
{
    float time = 1.0f;

    // 角で同時に複数の面へ接触する場合も保持する
    DirectX::XMFLOAT3 normals[3] = {};
    int normalCount = 0;
};

class Collision
{
  public:

    static SweepStatus SweepAABB(const AABB& movingBox, const DirectX::XMFLOAT3& displacement,
                                 const AABB& obstacle, SweepHit& hit);

    static bool IntersectSphereVsSphere(const DirectX::XMFLOAT3& positionA, float radiusA,
                                        const DirectX::XMFLOAT3& positionB, float radiusB,
                                        DirectX::XMFLOAT3& outPositionB);

    static bool IntersectCylinderVsCylinder(const DirectX::XMFLOAT3& positionA, float radiusA, float heightA,
                                            const DirectX::XMFLOAT3& positionB, float radiusB, float heightB,
                                            DirectX::XMFLOAT3& outPositionB);

    static bool IntersectSphereVsCylinder(const DirectX::XMFLOAT3& spherePosition, float sphereRadius,
                                          const DirectX::XMFLOAT3& cylinderPosition, float cylinderRadius,
                                          float cylinderHeight, DirectX::XMFLOAT3& outCylinderPosition);

    static bool IntersectCapsuleVsCylinder(const DirectX::XMFLOAT3& capsuleStart,
                                           const DirectX::XMFLOAT3& capsuleEnd, float capsuleRadius,
                                           const DirectX::XMFLOAT3& cylinderPosition, float cylinderRadius,
                                           float cylinderHeight, DirectX::XMFLOAT3& outContactPosition);

    static bool IsHitAABBToAABB(const DirectX::XMFLOAT3& aMin, const DirectX::XMFLOAT3& aMax,
                                const DirectX::XMFLOAT3& bMin, const DirectX::XMFLOAT3& bMax);

    static bool PushBackFromMultipleAABB(
        const DirectX::XMFLOAT3& playerMin, const DirectX::XMFLOAT3& playerMax,
        const std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>& blocks,
        DirectX::XMFLOAT3& totalOffset);

    static bool PushBackAABB(const DirectX::XMFLOAT3& minA, const DirectX::XMFLOAT3& maxA,
                             const DirectX::XMFLOAT3& minB, const DirectX::XMFLOAT3& maxB,
                             DirectX::XMFLOAT3& outOffset);

    static void TransformAABB(const DirectX::XMFLOAT4X4& transform, const DirectX::XMFLOAT3& localMin,
                              const DirectX::XMFLOAT3& localMax, DirectX::XMFLOAT3& outWorldMin,
                              DirectX::XMFLOAT3& outWorldMax);
};
