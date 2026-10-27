#include "../Source/CameraSectionState.h"
#include "../Source/CameraBoundsRegion.h"
#include <cassert>
#include <iostream>

static CameraLimitZoneData Zone(float left, float right, const char* section, bool trigger = false)
{
    CameraLimitZoneData zone;
    zone.minX = left;
    zone.maxX = right;
    zone.minY = 0;
    zone.maxY = 20;
    zone.cameraSection = section;
    zone.isSectionTrigger = trigger;
    return zone;
}

int main()
{
    std::vector<CameraLimitZoneData> zones = {Zone(0, 40, "first"),         Zone(40, 80, "second"),
                                              Zone(80, 100, "third"),       Zone(100, 120, "third"),
                                              Zone(38, 40, "first", true),  Zone(40, 42, "second", true),
                                              Zone(78, 80, "second", true), Zone(80, 82, "third", true)};
    CameraSectionState state;
    assert(!state.Update(zones, 10, 10));
    assert(state.GetSection() == "first" && state.GetBounds(zones).size() == 1);
    assert(state.Update(zones, 41, 10));
    assert(state.GetSection() == "second");
    assert(state.Update(zones, 81, 10));
    assert(state.GetSection() == "third" && state.GetBounds(zones).size() == 2);
    assert(!state.Update(zones, 110, 10));
    assert(!state.Update(zones, 110, 30));
    assert(state.Update(zones, 79, 10));
    assert(state.GetSection() == "second");
    assert(!state.Update(zones, 80, 10));
    assert(state.Update(zones, 39, 10));
    assert(state.GetSection() == "first");
    assert(!state.Update(zones, 110, 10, true));
    assert(state.GetSection() == "third");
    CameraBoundsRegion region;
    region.SetBounds(state.GetBounds(zones));
    assert(region.ContainsPlayer(110, 10));
    assert(!region.ContainsPlayer(60, 10));

    zones = {Zone(0, 40, ""), Zone(40, 80, "")};
    state.Update(zones, 10, 10, true);
    assert(state.GetBounds(zones).size() == 2);
    assert(!state.Update(zones, 60, 10));

    std::vector<CameraBoundsRegion::Rect> course = {{118.9375f, 159, 13.0625f, 29.875f},
                                                    {158.8125f, 179.0625f, 20, 33.9375f},
                                                    {171, 179.375f, 12, 20.125f},
                                                    {179.25f, 240.9375f, 11.6875f, 33.9375f}};
    region.SetBounds(course);
    const float path[][2] = {{158, 23}, {159, 23}, {160, 23},    {175, 23}, {175, 20.05f},
                             {175, 19}, {175, 18}, {179.3f, 18}, {180, 18}, {185, 18}};

    for (const auto& point : path)
    {
        float x = point[0], y = point[1];
        assert(region.ContainsPlayer(x, y));
        region.ConstrainSectionFollow(x, y, x - 1, y + 1, point[0], point[1], false);
        assert(x == point[0]);
        assert(y >= point[1]);
        assert(y <= point[1] - 0.5f + 4.622f);
    }

    float x = 200, y = 0;
    region.ConstrainSectionFollow(x, y, 185, 18, 200, 0, false);
    assert(x == 185 && y == 18);

    region.SetBounds(
        std::vector<CameraBoundsRegion::Rect>{{-2.125f, 72.0625f, -3, 19.875f}, {30, 38.0625f, -8, 3.75f}});

    for (float playerX = 7; playerX <= 64; playerX += 0.25f)
    {
        x = playerX;
        y = -2;
        region.ConstrainSectionFollow(x, y, x, y, playerX, -0.5f, false);
        assert(y >= -3 + 4.622f - 0.001f);
    }

    x = 34;
    y = -5;
    region.ConstrainSectionFollow(x, y, x, y, 34, -5.5f, false);
    assert(y <= -5.5f - 0.5f + 4.622f);

    region.SetBounds(std::vector<CameraBoundsRegion::Rect>{{0, 40, -3, 20}, {30, 100, -8, 20}});
    x = 70;
    y = -5;
    region.ConstrainSectionFollow(x, y, x, y, 70, -5, false);
    assert(std::abs(y - (-8 + 4.622f)) < 0.001f);
    std::cout << "PASS: section triggers, reverse travel, seams, outside, warp and legacy data\n";
}
