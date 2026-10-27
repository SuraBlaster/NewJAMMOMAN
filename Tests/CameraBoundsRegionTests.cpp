#include "../Source/CameraBoundsRegion.h"
#include <cassert>
#include <cmath>
#include <iostream>

using Rect = CameraBoundsRegion::Rect;

static void Check(CameraBoundsRegion& region, float x, float y, float expectedX, float expectedY)
{
    region.Constrain(x, y);
    assert(std::abs(x - expectedX) < 0.001f);
    assert(std::abs(y - expectedY) < 0.001f);
}

int main()
{
    CameraBoundsRegion region;
    region.SetBounds(std::vector<Rect>{{0, 40, 0, 20}});
    Check(region, -10, 10, 8.176f, 10);
    Check(region, 50, 30, 31.824f, 15.662f);

    region.SetBounds(std::vector<Rect>{{0, 10, 0, 20}, {10, 20, 0, 20}});
    assert(region.HasFittingViewport());
    Check(region, 10, 10, 10, 10);

    region.SetBounds(std::vector<Rect>{{20, 40, 0, 20}, {0, 20, 0, 20}});

    for (float x = 18; x <= 22; x += 0.1f)
    {
        Check(region, x, 10, x, 10);
    }

    region.SetBounds(std::vector<Rect>{{0, 50, 0, 20}, {0, 20, 0, 50}});
    Check(region, 20, 20, 20, 15.662f);
    Check(region, 10, 35, 10, 35);

    region.SetBounds(std::vector<Rect>{{0, 60, 0, 20}, {0, 60, 40, 60}, {0, 20, 20, 40}, {40, 60, 20, 40}});
    Check(region, 30, 22, 30, 15.662f);

    region.SetBounds(std::vector<Rect>{{0, 4, 0, 4}});
    assert(!region.HasFittingViewport());
    Check(region, 100, 100, 2, 2.142f);
    region.SetBounds(std::vector<Rect>{});
    Check(region, 100, 100, 100, 100);

    region.SetBounds(std::vector<Rect>{{0, 40, 0, 20}, {40, 100, 0, 6}});

    for (float playerX = 10; playerX <= 100; playerX += 0.25f)
    {
        float x = playerX + 2, y = 3;
        region.ConstrainKeepingPlayerVisible(x, y, playerX, 1);
        assert(playerX >= x - 8.176f + 1.5f - 0.001f);
        assert(playerX <= x + 8.176f - 1.5f + 0.001f);
        assert(1 >= y - 4.622f + 0.5f - 0.001f);
        assert(3 <= y + 4.338f - 0.5f + 0.001f);
    }

    region.SetBounds(std::vector<Rect>{{0, 50, 0, 20}, {0, 20, 0, 50}});
    float x = 20, y = 20;
    region.ConstrainKeepingPlayerVisible(x, y, 10, 19);
    assert(std::abs(x - 11.824f) < 0.001f && y == 20);

    region.SetBounds(std::vector<Rect>{{0, 4, 0, 4}});
    x = 100;
    y = -100;
    region.ConstrainKeepingPlayerVisible(x, y, 2, 4);
    assert(x >= 2 - 8.176f + 1.5f - 0.001f);
    assert(y >= 6.5f - 4.338f - 0.001f);

    x = 100;
    y = -100;
    region.ConstrainKeepingPlayerVisible(x, y, 100, 50);
    assert(std::abs(x - 2) < 0.001f && std::abs(y - 2.142f) < 0.001f);

    region.SetBounds(std::vector<Rect>{{0, 40, 0, 20}});

    for (const auto& player :
         std::vector<Rect>{{-20, 0, 10, 0}, {70, 0, 10, 0}, {20, 0, -20, 0}, {20, 0, 50, 0}})
    {
        x = player.minX + 2;
        y = player.minY;
        float expectedX = x, expectedY = y;
        region.Constrain(expectedX, expectedY);
        region.ConstrainKeepingPlayerVisible(x, y, player.minX, player.minY);
        assert(x == expectedX && y == expectedY);
    }

    region.SetBounds(std::vector<Rect>{{0, 60, 0, 20}, {0, 60, 40, 60}, {0, 20, 20, 40}, {40, 60, 20, 40}});
    x = 30;
    y = 30;
    float expectedX = x, expectedY = y;
    region.Constrain(expectedX, expectedY);
    region.ConstrainKeepingPlayerVisible(x, y, 30, 30);
    assert(x == expectedX && y == expectedY);
    region.SetBounds(std::vector<Rect>{});
    x = 100;
    y = 50;
    region.ConstrainKeepingPlayerVisible(x, y, 100, 50);
    assert(x == 100 && y == 50);

    region.SetBounds(std::vector<Rect>{{0, 40, 0, 20}});
    x = 2;
    y = 10;
    region.ConstrainKeepingPlayerVisible(x, y, 2, 10);
    assert(std::abs(x - 8.176f) < 0.001f && y == 10);
    std::cout << "PASS: union, seams, L shape, hole, narrow and empty bounds\n";
}
