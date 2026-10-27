#pragma once

#include <algorithm>
#include <limits>
#include <vector>

class CameraBoundsRegion
{
  public:

    struct Rect
    {
        float minX, maxX, minY, maxY;
    };

    template <class Zones> void SetBounds(const Zones& zones)
    {
        std::vector<Rect> next;

        for (const auto& zone : zones)
        {
            next.push_back({zone.minX, zone.maxX, zone.minY, zone.maxY});
        }

        if (Equal(next, bounds))
        {
            return;
        }

        bounds = next;
        Build();
    }

    void Constrain(float& x, float& y) const
    {
        const auto& candidates = allowed.empty() ? fallback : allowed;
        float bestDistance = (std::numeric_limits<float>::max)();
        float bestX = x, bestY = y;

        for (const auto& rect : candidates)
        {
            const float candidateX = std::clamp(x, rect.minX, rect.maxX);
            const float candidateY = std::clamp(y, rect.minY, rect.maxY);
            const float dx = candidateX - x, dy = candidateY - y;
            const float distance = dx * dx + dy * dy;

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestX = candidateX;
                bestY = candidateY;
            }
        }

        x = bestX;
        y = bestY;
    }

    bool HasFittingViewport() const
    {
        return !allowed.empty();
    }

    bool ContainsPlayer(float x, float y) const
    {
        return std::any_of(bounds.begin(), bounds.end(), [x, y](const Rect& rect)
                           { return x >= rect.minX && x <= rect.maxX && y >= rect.minY && y <= rect.maxY; });
    }

    void KeepPlayerVisible(float& x, float& y, float playerX, float playerY) const
    {
        x = std::clamp(x, playerX - HALF_WIDTH + 1.5f, playerX + HALF_WIDTH - 1.5f);
        y = std::clamp(y, playerY + 2.5f - ABOVE, playerY - 0.5f + BELOW);
    }

    void ConstrainSectionFollow(float& x, float& y, float currentX, float currentY, float playerX,
                                float playerY, bool enteringSection, float elapsedTime = 0.0f) const
    {
        if (bounds.empty())
        {
            return;
        }

        const bool inside = ContainsPlayer(playerX, playerY);

        if (!inside && !enteringSection)
        {
            x = currentX;
            y = currentY;
            return;
        }

        const float followY = y;
        Rect envelope = bounds.front();

        for (const auto& rect : bounds)
        {
            envelope.minX = (std::min)(envelope.minX, rect.minX);
            envelope.maxX = (std::max)(envelope.maxX, rect.maxX);
            envelope.minY = (std::min)(envelope.minY, rect.minY);
            envelope.maxY = (std::max)(envelope.maxY, rect.maxY);
        }

        envelope.minX += HALF_WIDTH;
        envelope.maxX -= HALF_WIDTH;
        envelope.minY += BELOW;
        envelope.maxY -= ABOVE;

        if (envelope.minX > envelope.maxX)
        {
            envelope.minX = envelope.maxX = (envelope.minX + envelope.maxX) * 0.5f;
        }

        if (envelope.minY > envelope.maxY)
        {
            envelope.minY = envelope.maxY = (envelope.minY + envelope.maxY) * 0.5f;
        }

        x = std::clamp(x, envelope.minX, envelope.maxX);
        y = std::clamp(y, envelope.minY, envelope.maxY);
        y = (std::max)(y, ViewportFloor(x) + BELOW);

        if (inside && !enteringSection && elapsedTime > 0.0f &&
            (y < followY - 0.0001f || y > followY + 0.0001f))
        {
            const float normalStep = followY - currentY;
            const float correctionStep = 4.0f * elapsedTime;
            y = currentY + std::clamp(y - currentY, (std::min)(normalStep, -correctionStep),
                                      (std::max)(normalStep, correctionStep));
        }

        if (inside)
        {
            KeepPlayerVisible(x, y, playerX, playerY);
        }
    }

    void ConstrainKeepingPlayerVisible(float& x, float& y, float playerX, float playerY) const
    {

        if (!ContainsPlayer(playerX, playerY))
        {
            Constrain(x, y);
            return;
        }

        const Rect visible = {playerX - HALF_WIDTH + 1.5f, playerX + HALF_WIDTH - 1.5f,
                              playerY + 2.5f - ABOVE, playerY - 0.5f + BELOW};
        const auto& candidates = allowed.empty() ? fallback : allowed;
        float bestDistance = (std::numeric_limits<float>::max)();
        float bestX = x, bestY = y;
        bool found = false;

        for (const auto& rect : candidates)
        {
            const Rect intersection = {
                (std::max)(rect.minX, visible.minX), (std::min)(rect.maxX, visible.maxX),
                (std::max)(rect.minY, visible.minY), (std::min)(rect.maxY, visible.maxY)};

            if (intersection.minX > intersection.maxX || intersection.minY > intersection.maxY)
            {
                continue;
            }

            const float candidateX = std::clamp(x, intersection.minX, intersection.maxX);
            const float candidateY = std::clamp(y, intersection.minY, intersection.maxY);
            const float dx = candidateX - x, dy = candidateY - y;
            const float distance = dx * dx + dy * dy;

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestX = candidateX;
                bestY = candidateY;
                found = true;
            }
        }

        if (!found)
        {

            bestX = x;
            bestY = y;
            Constrain(bestX, bestY);
        }

        x = std::clamp(bestX, visible.minX, visible.maxX);
        y = std::clamp(bestY, visible.minY, visible.maxY);
    }

  private:

    static constexpr float HALF_WIDTH = 8.176f;
    static constexpr float BELOW = 4.622f;
    static constexpr float ABOVE = 4.338f;
    std::vector<Rect> bounds, allowed, fallback;

    float ViewportFloor(float x) const
    {

        std::vector<float> edges = {x - HALF_WIDTH, x + HALF_WIDTH};

        for (const auto& rect : bounds)
        {
            if (rect.minX > edges.front() && rect.minX < x + HALF_WIDTH)
            {
                edges.push_back(rect.minX);
            }

            if (rect.maxX > x - HALF_WIDTH && rect.maxX < x + HALF_WIDTH)
            {
                edges.push_back(rect.maxX);
            }
        }

        std::sort(edges.begin(), edges.end());
        float floor = -(std::numeric_limits<float>::max)();

        for (size_t i = 1; i < edges.size(); ++i)
        {
            if (edges[i] == edges[i - 1])
            {
                continue;
            }

            const float sample = (edges[i - 1] + edges[i]) * 0.5f;
            float stripFloor = (std::numeric_limits<float>::max)();

            for (const auto& rect : bounds)
            {
                if (sample >= rect.minX && sample <= rect.maxX)
                {
                    stripFloor = (std::min)(stripFloor, rect.minY);
                }
            }

            if (stripFloor != (std::numeric_limits<float>::max)())
            {
                floor = (std::max)(floor, stripFloor);
            }
        }

        return floor;
    }

    static bool Equal(const std::vector<Rect>& a, const std::vector<Rect>& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (a[i].minX != b[i].minX || a[i].maxX != b[i].maxX || a[i].minY != b[i].minY ||
                a[i].maxY != b[i].maxY)
            {
                return false;
            }
        }

        return true;
    }

    void Subtract(const Rect& hole)
    {
        std::vector<Rect> remaining;

        for (const auto& rect : allowed)
        {
            const float left = (std::max)(rect.minX, hole.minX);
            const float right = (std::min)(rect.maxX, hole.maxX);
            const float bottom = (std::max)(rect.minY, hole.minY);
            const float top = (std::min)(rect.maxY, hole.maxY);

            if (rect.maxX <= hole.minX || rect.minX >= hole.maxX || rect.maxY <= hole.minY ||
                rect.minY >= hole.maxY)
            {
                remaining.push_back(rect);
                continue;
            }

            if (rect.minX <= left && left == hole.minX)
            {
                remaining.push_back({rect.minX, left, rect.minY, rect.maxY});
            }

            if (right <= rect.maxX && right == hole.maxX)
            {
                remaining.push_back({right, rect.maxX, rect.minY, rect.maxY});
            }

            if (rect.minY <= bottom && bottom == hole.minY)
            {
                remaining.push_back({left, right, rect.minY, bottom});
            }

            if (top <= rect.maxY && top == hole.maxY)
            {
                remaining.push_back({left, right, top, rect.maxY});
            }
        }

        allowed.swap(remaining);
    }

    void Build()
    {
        allowed.clear();
        fallback.clear();

        if (bounds.empty())
        {
            return;
        }

        std::vector<float> xs, ys;

        for (const auto& rect : bounds)
        {
            xs.push_back(rect.minX);
            xs.push_back(rect.maxX);
            ys.push_back(rect.minY);
            ys.push_back(rect.maxY);
            Rect inset = {rect.minX + HALF_WIDTH, rect.maxX - HALF_WIDTH, rect.minY + BELOW,
                          rect.maxY - ABOVE};

            if (inset.minX > inset.maxX)
            {
                inset.minX = inset.maxX = (rect.minX + rect.maxX) * 0.5f;
            }

            if (inset.minY > inset.maxY)
            {
                inset.minY = inset.maxY = (rect.minY + rect.maxY + BELOW - ABOVE) * 0.5f;
            }

            fallback.push_back(inset);
        }

        const auto sortUnique = [](std::vector<float>& values)
        {

            std::sort(values.begin(), values.end());
            values.erase(std::unique(values.begin(), values.end()), values.end());
        };
        sortUnique(xs);
        sortUnique(ys);
        Rect envelope = {xs.front() + HALF_WIDTH, xs.back() - HALF_WIDTH, ys.front() + BELOW,
                         ys.back() - ABOVE};

        if (envelope.minX > envelope.maxX || envelope.minY > envelope.maxY)
        {
            return;
        }

        allowed.push_back(envelope);

        for (size_t i = 1; i < xs.size(); ++i)
        {
            for (size_t j = 1; j < ys.size(); ++j)
            {
                const float x = (xs[i - 1] + xs[i]) * 0.5f;
                const float y = (ys[j - 1] + ys[j]) * 0.5f;
                bool covered = false;

                for (const auto& rect : bounds)
                {
                    if (x >= rect.minX && x <= rect.maxX && y >= rect.minY && y <= rect.maxY)
                    {
                        covered = true;
                        break;
                    }
                }

                if (!covered)
                {
                    Subtract({xs[i - 1] - HALF_WIDTH, xs[i] + HALF_WIDTH, ys[j - 1] - ABOVE, ys[j] + BELOW});
                }
            }
        }
    }
};
