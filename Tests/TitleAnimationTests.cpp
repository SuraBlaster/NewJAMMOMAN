#include "TitleAnimation.h"
#include <cassert>
#include <cstdio>

int main()
{
    namespace A = TitleAnimation;
    const auto start = A::Evaluate(0);
    assert(!A::VisibleEdge(start, 0).visible && start.xAlpha == 0);
    const auto drawn = A::Evaluate(A::DrawEnd);
    for (int edge = 0; edge < 3; ++edge)
    {
        const auto segment = A::VisibleEdge(drawn, edge);
        assert(segment.visible);
        assert(A::Length(segment.to, drawn.vertices[(edge + 2) % 3]) < 0.001f);
    }
    const auto morphed = A::Evaluate(A::MorphEnd);
    const auto held = A::Evaluate(A::HoldEnd);
    assert(held.vertices[0].x == 0 && held.vertices[1].x == 1280);
    assert(held.vertices[2].y > held.vertices[0].y);
    // The long edge must turn counterclockwise without reversing the face.
    auto previous = held;
    float totalTurn = 0;
    for (int i = 1; i <= 200; ++i)
    {
        const auto current = A::Evaluate(A::HoldEnd + (A::MorphEnd - A::HoldEnd) * i / 200);
        const A::Point a = {previous.vertices[1].x - previous.vertices[0].x,
                            previous.vertices[1].y - previous.vertices[0].y};
        const A::Point b = {current.vertices[1].x - current.vertices[0].x,
                            current.vertices[1].y - current.vertices[0].y};
        const A::Point c = {current.vertices[2].x - current.vertices[0].x,
                            current.vertices[2].y - current.vertices[0].y};
        assert(b.x * c.y - b.y * c.x > 0);
        const float turn = std::atan2(a.x * b.y - a.y * b.x, a.x * b.x + a.y * b.y);
        assert(turn < 0);
        for (int corner = 0; corner < 3; ++corner)
            assert(A::Length(previous.vertices[corner], current.vertices[corner]) < 25);
        totalTurn += turn;
        previous = current;
    }
    assert(totalTurn < -3.1f && totalTurn > -3.4f);
    const auto small = A::Evaluate(2.55f);
    const auto recovering = A::Evaluate(2.75f);
    const float smallEdge = A::Length(small.vertices[0], small.vertices[1]);
    assert(smallEdge < A::Length(held.vertices[0], held.vertices[1]) * 0.5f);
    assert(smallEdge < A::Length(recovering.vertices[0], recovering.vertices[1]));
    assert(A::Length(recovering.vertices[0], recovering.vertices[1])
        < A::Length(morphed.vertices[0], morphed.vertices[1]));
    assert(A::Length(morphed.vertices[2], A::TriangleOrigin) < 0.001f);
    assert(morphed.lineAlpha == 1 && morphed.triangleAlpha == 0);
    const auto swapped = A::Evaluate(A::SwapEnd);
    assert(swapped.lineAlpha == 0 && swapped.triangleAlpha == 1 && swapped.logoProgress == 0);
    const auto logo = A::Evaluate(A::LogoEnd);
    assert(logo.logoProgress == 1 && logo.xAlpha == 0 && logo.xScale == 7);
    const auto done = A::Evaluate(A::IntroEnd);
    assert(done.xScale == 1 && done.xAlpha == 1 && done.impactAlpha == 0);
    float previousScale = 7;
    for (int i = 0; i <= 480; ++i)
    {
        const auto state = A::Evaluate(i / 100.0f);
        assert(state.lineAlpha >= 0 && state.lineAlpha <= 1);
        assert(std::abs(state.lineAlpha + state.triangleAlpha - 1) < 0.001f);
        assert(state.xScale <= previousScale && state.xScale >= 1);
        previousScale = state.xScale;
        for (auto p : state.vertices) assert(std::isfinite(p.x) && std::isfinite(p.y));
    }
    puts("Title animation boundaries and progression: PASS");
}
