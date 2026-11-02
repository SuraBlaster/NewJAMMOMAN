#include <cassert>
#include <cstdio>
#include <cmath>
#include "AttackAssistHitbox.h"

int main()
{
    using namespace AttackAssist;
    const DirectX::XMFLOAT3 origin{10.0f, 3.0f, 2.0f};
    for (int step = 1; step <= 3; ++step)
    {
        const auto& settings = COMBO_SETTINGS[step - 1];
        assert(!Build(origin, 1, step, settings.activeStart - 0.001f));
        assert(!Build(origin, 1, step, settings.activeEnd));
        const auto right = Build(origin, 1, step, settings.activeStart);
        const auto left = Build(origin, -1, step, settings.activeStart);
        assert(right && left);
        assert(std::abs(right->max.x - origin.x - 1.80f) < 0.0001f);
        assert(std::abs(left->min.x + right->max.x - 2 * origin.x) < 0.0001f);
        assert(std::abs(left->max.x + right->min.x - 2 * origin.x) < 0.0001f);
        // 刃の向きや奥行きに関係なく、前方の同じ間合いを3段とも拾う。
        assert(IntersectsCylinder(*right, {11.6f, 3.0f, 2.7f}, 0.2f, 1.0f));
        assert(IntersectsCylinder(*left, {8.4f, 3.0f, 1.3f}, 0.2f, 1.0f));
        assert(!IntersectsCylinder(*right, {8.4f, 3.0f, 2.0f}, 0.2f, 1.0f));
        assert(!IntersectsCylinder(*left, {11.6f, 3.0f, 2.0f}, 0.2f, 1.0f));
        assert(!IntersectsCylinder(*right, {12.2f, 3.0f, 2.0f}, 0.2f, 1.0f));
        assert(!IntersectsCylinder(*right, {11.0f, 3.0f, 3.2f}, 0.2f, 1.0f));
        assert(!IntersectsCylinder(*right, {11.0f, 4.6f, 2.0f}, 0.2f, 1.0f));
        assert(!IntersectsCylinder(*right, {11.0f, 1.0f, 2.0f}, 0.2f, 1.0f));
        // X/Z各軸では近くても、箱の角から半径以上なら外す。
        assert(!IntersectsCylinder(*right, {right->max.x + 0.15f, 3.0f,
                                            right->max.z + 0.15f}, 0.2f, 1.0f));
    }
    assert(!Build(origin, 1, 0, 0.1f));
    assert(!Build(origin, 1, 4, 0.1f));
    std::puts("AttackAssistHitbox tests passed");
}
