#include "PlayerTrail.h"
#include <cassert>
#include <cstdio>

static bool Equal(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

int main()
{
    PlayerTrail trail;
    const DirectX::XMFLOAT3 rootA{1, 2, 3}, tipA{4, 5, 6};
    const DirectX::XMFLOAT3 rootB{7, 8, 9}, tipB{10, 11, 12};
    trail.RecordSword(rootA, tipA, true);
    assert(Equal(trail.GetPreviousRoot(), rootA));
    assert(Equal(trail.GetPreviousTip(), tipA));
    trail.RecordSword(rootB, tipB, false);
    assert(Equal(trail.GetPreviousRoot(), rootA));
    assert(Equal(trail.GetPreviousTip(), tipA));
    trail.RecordSword(rootB, tipB, true);
    assert(Equal(trail.GetPreviousRoot(), rootB));
    assert(Equal(trail.GetPreviousTip(), tipB));
    trail.FadeSword(1.0f);
    assert(Equal(trail.GetPreviousRoot(), rootB));

    // GPU-free model dependency: this test checks snapshot timing and data.
    Model model(nullptr, "test", 60.0f);
    const DirectX::XMFLOAT3 rotation{0, 1, 0}, scale{1, 1, 1};
    trail.RecordMotion(0.01f, model, rootA, rotation, scale);
    assert(trail.GetMotionSnapshots().size() == 1);
    const auto& snapshot = trail.GetMotionSnapshots().back();
    assert(Equal(snapshot.position, rootA) && Equal(snapshot.angle, rotation));
    assert(Equal(snapshot.scale, scale) && snapshot.alpha == 0.5f);
    assert(snapshot.trailModel && snapshot.trailModel.get() != &model);
    trail.RecordMotion(0.01f, model, rootB, rotation, scale);
    assert(trail.GetMotionSnapshots().size() == 1);
    for (int i = 0; i < 4; ++i)
        trail.RecordMotion(0.06f, model, {float(i), 0, 0}, rotation, scale);
    assert(trail.GetMotionSnapshots().size() == 3);
    assert(trail.GetMotionSnapshots().front().position.x == 1);
    assert(trail.GetMotionSnapshots().back().position.x == 3);
    trail.ClearMotion();
    assert(trail.GetMotionSnapshots().empty());
    trail.RecordMotion(0.01f, model, rootA, rotation, scale);
    assert(trail.GetMotionSnapshots().empty());
    std::puts("Player trail history, reset, snapshot cadence and capacity: PASS");
}
