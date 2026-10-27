#include "../Source/CameraBoundsRegion.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{

    const std::vector<CameraBoundsRegion::Rect> shifted = {{125.9375f, 166.0f, 13.0625f, 29.875f},
                                                           {165.8125f, 186.0625f, 20.0f, 33.9375f},
                                                           {178.0f, 186.375f, 12.0f, 20.125f},
                                                           {186.25f, 247.9375f, 11.6875f, 33.9375f}};
    CameraBoundsRegion region;
    region.SetBounds(shifted);

    float oldX = 158.0f, oldY = 18.0f;
    region.ConstrainSectionFollow(oldX, oldY, 157.9f, 18.0f, 158.0f, 18.0f, false);
    assert(oldY > 21.0f);

    for (int fps : {30, 60, 120})
    {
        const float dt = 1.0f / fps;
        float currentY = 18.0f;

        for (int frame = 0; frame < fps; frame++)
        {
            float x = 158.0f, y = currentY + (18.0f - currentY) * (1.0f - std::exp(-3.0f * dt));
            region.ConstrainSectionFollow(x, y, x, currentY, 158.0f, 18.0f, false, dt);
            assert(std::abs(y - currentY) <= 4.0f * dt + 0.0001f);
            assert(18.0f >= y - 4.622f && 19.4f <= y + 4.338f);
            currentY = y;
        }

        assert(currentY > 21.9f);

        float x = 185.0f, y = 23.0f;
        region.ConstrainSectionFollow(x, y, x, 24.0f, 185.0f, 15.5f, false, dt);
        assert(y <= 15.5f - 0.5f + 4.622f + 0.0001f);
    }

    auto original = shifted;

    for (auto& r : original)
    {
        r.minX -= 7;
        r.maxX -= 7;
    }

    CameraBoundsRegion unshifted;
    unshifted.SetBounds(original);
    float x1 = 158.0f, y1 = 18.0f, x2 = 151.0f, y2 = 18.0f;
    region.ConstrainSectionFollow(x1, y1, 157.9f, 18, 158.0f, 18, false, 1.0f / 60);
    unshifted.ConstrainSectionFollow(x2, y2, 150.9f, 18, 151.0f, 18, false, 1.0f / 60);
    assert(std::abs(y1 - y2) < 0.0001f && std::abs(x1 - x2 - 7) < 0.0001f);
    std::cout
        << "PASS: shifted bounds reproduce snap; correction continuous at 30/60/120 FPS; player visible; translation invariant\n";
}
