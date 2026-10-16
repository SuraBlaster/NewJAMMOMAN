#pragma once

#include <algorithm>

#include "Camera.h"
#include "ViewVolume.h"

inline bool IsEnemyInView(const ViewVolume& volume, const DirectX::XMFLOAT3& position, float radius,
                          float height)
{
    const DirectX::BoundingBox body({position.x, position.y + height * 0.5f, position.z},
                                    {radius, (std::max)(height * 0.5f, radius), radius});

    return volume.Intersects(body);
}

inline bool IsEnemyOnScreen(const DirectX::XMFLOAT3& position, float radius, float height)
{
    const auto& camera = Camera::Instance();
    const ViewVolume volume(camera.GetView(), camera.GetProjection());

    return IsEnemyInView(volume, position, radius, height);
}
