#pragma once
#include <algorithm>
#include <cmath>

// Rendering-independent timing and geometry for the 1280 x 720 design canvas.
namespace TitleAnimation
{
    constexpr float DrawEnd = 0.8f;
    constexpr float HoldEnd = 2.05f;
    constexpr float MorphEnd = 2.9f;
    constexpr float SwapEnd = 3.1f;
    constexpr float LogoEnd = 3.7f;
    constexpr float XEnd = 4.5f;
    constexpr float IntroEnd = 4.8f;
    struct Point { float x, y; };
    constexpr Point TriangleOrigin = {150, 235};
    constexpr float TriangleWidth = 980;
    constexpr float TriangleHeight = TriangleWidth * 809.0f / 6013.0f;
    inline float Clamp(float t) { return (std::max)(0.0f, (std::min)(1.0f, t)); }
    inline float Progress(float t, float start, float end) { return Clamp((t - start) / (end - start)); }
    inline float Smooth(float t) { t = Clamp(t); return t * t * (3 - 2 * t); }
    inline Point Lerp(Point a, Point b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }
    inline float Length(Point a, Point b) { return std::hypot(b.x - a.x, b.y - a.y); }
    struct State
    {
        Point vertices[3];
        float drawProgress, lineAlpha, triangleAlpha, logoProgress, xScale, xAlpha, impactAlpha;
    };
    inline State Evaluate(float time)
    {
        const Point initial[] = {{0, 365}, {1280, 365}, {320, 580}};
        // Match Triangle.png's silhouette: top left, top right, lower tip.
        const Point final[] = {
            TriangleOrigin,
            {TriangleOrigin.x + TriangleWidth, TriangleOrigin.y + TriangleHeight * (71.0f / 809)},
            {TriangleOrigin.x + TriangleWidth * (741.0f / 6013), TriangleOrigin.y + TriangleHeight}};
        State state = {};
        // Rotate in the screen plane, preserving the face's winding throughout.
        const float morph = Smooth(Progress(time, HoldEnd, MorphEnd));
        // Shrink around the moving center, then recover before the image swap.
        constexpr float shrinkEnd = 2.55f;
        constexpr float minimumScale = 0.45f;
        const float shrink = time < shrinkEnd
            ? Smooth(Progress(time, HoldEnd, shrinkEnd))
            : 1 - Smooth(Progress(time, shrinkEnd, MorphEnd));
        const float scale = 1 - (1 - minimumScale) * shrink;
        Point initialCenter = {}, finalCenter = {};
        for (int i = 0; i < 3; ++i)
        {
            initialCenter.x += initial[i].x / 3;
            initialCenter.y += initial[i].y / 3;
            finalCenter.x += final[i].x / 3;
            finalCenter.y += final[i].y / 3;
        }
        const Point center = Lerp(initialCenter, finalCenter, morph);
        // Screen Y points down: -PI is a counterclockwise half turn.
        const float angle = -3.14159265359f * morph;
        const float cosine = std::cos(angle), sine = std::sin(angle);
        for (int i = 0; i < 3; ++i)
        {
            // Cycle corners to preserve winding; undo the target half turn
            // in local space so the rotation lands on the image continuously.
            const Point target = final[(i + 1) % 3];
            const Point local = Lerp(
                {initial[i].x - initialCenter.x, initial[i].y - initialCenter.y},
                {finalCenter.x - target.x, finalCenter.y - target.y}, morph);
            state.vertices[i] = {center.x + (local.x * cosine - local.y * sine) * scale,
                                 center.y + (local.x * sine + local.y * cosine) * scale};
            if (morph == 0) state.vertices[i] = initial[i];
            if (morph == 1) state.vertices[i] = target;
        }
        state.drawProgress = Progress(time, 0, DrawEnd);
        state.triangleAlpha = Smooth(Progress(time, MorphEnd, SwapEnd));
        state.lineAlpha = 1 - state.triangleAlpha;
        state.logoProgress = Smooth(Progress(time, SwapEnd, LogoEnd));
        const float x = Progress(time, LogoEnd, XEnd);
        // Perspective-like recession: a near-camera X shrinks quickly, then settles.
        state.xScale = 1 + 6 * std::pow(1 - x, 3.0f);
        state.xAlpha = Progress(time, LogoEnd, LogoEnd + 0.14f);
        state.impactAlpha = time >= XEnd ? 1 - Progress(time, XEnd, IntroEnd) : 0;
        return state;
    }
    struct Segment { Point from, to; bool visible; };
    inline Segment VisibleEdge(const State& state, int edge)
    {
        float lengths[3];
        float perimeter = 0;
        for (int i = 0; i < 3; ++i) perimeter += lengths[i] = Length(state.vertices[(i + 1) % 3], state.vertices[(i + 2) % 3]);
        float remaining = perimeter * state.drawProgress;
        for (int i = 0; i < edge; ++i) remaining -= lengths[i];
        const Point a = state.vertices[(edge + 1) % 3], b = state.vertices[(edge + 2) % 3];
        return {a, Lerp(a, b, lengths[edge] > 0 ? Clamp(remaining / lengths[edge]) : 0), remaining > 0};
    }
}
