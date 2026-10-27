#include "../Source/EnemyActivation.h"
#include "../Source/AnimationKeyframeLookup.h"
#include "../Source/ViewVolume.h"
#include <cassert>
#include <iostream>
#include <vector>

struct Key
{
    float seconds;
    float value;
};

int main()
{
    EnemyActivation activation;

    for (int i = 0; i < 600; i++)
    {
        assert(!activation.Update(100, 20, 0, 0));
    }

    assert(!activation.Update(100, 20, 95, 0));
    assert(!activation.Update(100, 20, 92.9f, 20));
    assert(activation.Update(100, 20, 93, 17));
    assert(activation.Update(200, 20, 0, 0));
    EnemyActivation reverse;
    assert(reverse.Update(10, 5, 16, 7));
    std::vector<Key> keys;

    for (int i = 0; i < 4096; i++)
    {
        keys.push_back({i / 60.0f, float(i)});
    }

    size_t left = 0, right = 0;
    float rate = 0;

    for (int i = 0; i < 4095; i += 17)
    {
        const float t = (i + .3f) / 60.0f;
        assert(FindAnimationKeyframes(keys, t, left, right, rate));
        assert(left == size_t(i) && right == size_t(i + 1));
        assert(std::abs(rate - .3f) < .001f);
    }

    assert(FindAnimationKeyframes(keys, keys.back().seconds, left, right, rate));
    assert(right == 4095 && rate == 1);
    assert(!FindAnimationKeyframes(keys, -1, left, right, rate));
    assert(!FindAnimationKeyframes(std::vector<Key>{}, 0, left, right, rate));
    assert(FindAnimationKeyframes(std::vector<Key>{{0, 7}}, 10, left, right, rate) && left == right);

    DirectX::XMFLOAT4X4 view, projection;
    DirectX::XMStoreFloat4x4(&view, DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0, 0, -10, 1),
                                                              DirectX::XMVectorSet(0, 0, 0, 1),
                                                              DirectX::XMVectorSet(0, 1, 0, 0)));
    DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixOrthographicLH(19.2f, 10.8f, .1f, 100));
    ViewVolume volume(view, projection);
    assert(volume.Intersects(DirectX::BoundingBox({0, 0, 0}, {4, 4, 4})));
    assert(!volume.Intersects(DirectX::BoundingBox({61, 0, 0}, {4, 4, 4})));
    assert(volume.Intersects(DirectX::BoundingBox({11, 0, 0}, {4, 4, 4})));
    std::cout
        << "PASS: latched proximity, vertical separation, key lookup endpoints, offscreen culling and edge padding\n";
}
