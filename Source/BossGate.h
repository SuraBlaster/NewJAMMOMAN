#pragma once

#include <algorithm>
#include <cmath>
#include "Collision.h"
#include "StageData.h"

// Reusable shutter logic. Collision seals immediately when locked, while the
// visible shutter finishes closing. Never close an unlocked gate onto a body.
class BossGate
{
public:
    static constexpr int RIGHT_TYPE_ID = 301;
    static constexpr int LEFT_TYPE_ID = 302;

    enum class Direction { Right, Left };

    explicit BossGate(const AABB& bounds, Direction direction = Direction::Right)
        : bounds(bounds), direction(direction) {}

    static bool IsGateType(int typeId)
    {
        return typeId == RIGHT_TYPE_ID || typeId == LEFT_TYPE_ID;
    }

    // Tile-layer placements use the same center and one-metre size as terrain.
    // Join touching vertical cells so jumping through any row seals the column.
    static std::vector<BossGate> FromObjects(const std::vector<StageObjectData>& objects)
    {
        std::vector<StageObjectData> cells;
        for (const auto& object : objects)
            if (IsGateType(object.typeId)) cells.push_back(object);
        std::sort(cells.begin(), cells.end(), [](const auto& a, const auto& b) {
            if (a.typeId != b.typeId) return a.typeId < b.typeId;
            if (a.positionX != b.positionX) return a.positionX < b.positionX;
            return a.positionY < b.positionY;
        });
        std::vector<BossGate> gates;
        for (const auto& cell : cells)
        {
            const auto direction = cell.typeId == RIGHT_TYPE_ID ? Direction::Right : Direction::Left;
            const AABB box{
                { cell.positionX - 0.5f, cell.positionY - 0.5f * cell.scaleY, -0.5f },
                { cell.positionX + 0.5f, cell.positionY + 0.5f * cell.scaleY, 0.5f } };
            if (!gates.empty() && gates.back().direction == direction &&
                gates.back().bounds.min.x == box.min.x &&
                box.min.y <= gates.back().bounds.max.y + 0.0001f)
            {
                gates.back().bounds.max.y = (std::max)(gates.back().bounds.max.y, box.max.y);
            }
            else gates.emplace_back(box, direction);
        }
        return gates;
    }

    void Update(float elapsedTime, const AABB& body)
    {
        const bool verticalOverlap = body.max.y > bounds.min.y && body.min.y < bounds.max.y;
        const bool nearby = body.max.x >= bounds.min.x - APPROACH_DISTANCE &&
            body.min.x <= bounds.max.x + APPROACH_DISTANCE &&
            verticalOverlap;
        const bool entrySide = direction == Direction::Right ?
            body.max.x <= bounds.min.x : body.min.x >= bounds.max.x;
        const bool exitSide = direction == Direction::Right ?
            body.min.x >= bounds.max.x + CROSSING_MARGIN : body.max.x <= bounds.min.x - CROSSING_MARGIN;
        if (!locked && nearby && entrySide) approachedFromEntry = true;
        if (!locked && openness == 1.0f && approachedFromEntry && verticalOverlap && exitSide)
            Lock();
        // Reset the approach after backing away, without permanently locking.
        if (!locked && !nearby && entrySide) approachedFromEntry = false;
        const float target = !locked && nearby && approachedFromEntry ? 1.0f : 0.0f;
        const float step = (std::max)(0.0f, elapsedTime) / ANIMATION_DURATION;
        openness = target > openness ? (std::min)(target, openness + step) :
            (std::max)(target, openness - step);
    }

    void Lock() { locked = true; }
    bool IsLocked() const { return locked; }
    bool IsSolid() const { return locked || openness < 1.0f; }
    float GetOpenness() const { return openness; }
    const AABB& GetBounds() const { return bounds; }

private:
    static constexpr float APPROACH_DISTANCE = 2.0f;
    static constexpr float ANIMATION_DURATION = 0.35f;
    static constexpr float CROSSING_MARGIN = 0.05f;
    AABB bounds;
    Direction direction;
    float openness = 0.0f;
    bool locked = false;
    bool approachedFromEntry = false;
};
