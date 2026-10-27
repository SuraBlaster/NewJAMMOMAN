#include "../Source/Character.h"
#include "../Source/CollisionManager.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void Check(bool ok, const char* message)
{
    if (!ok)
    {
        throw std::runtime_error(message);
    }
}

class GroundProbe final : public Character
{
  public:

    int landings = 0;

    GroundProbe()
    {
        height = .5f;
        useGravity = true;
    }

    void Update(float dt) override
    {
        UpdateGroundPhysics(dt);
    }

    void MoveX(float x)
    {
        position.x = x;
    }

    void Jump()
    {
        velocity.y = 4;
    }

  private:

    void OnLanding() override
    {
        landings++;
    }
};

int main()
{
    auto& collision = CollisionManager::Instance();

    for (int fps : {30, 60, 120})
    {
        collision.ClearTerrainAABBs();
        collision.RegisterTerrainAABB({{0, -1.5f, -2}, {30, -.5f, 2}});
        GroundProbe vacuum;
        vacuum.SetPosition({10, -1.0625f, 0});

        for (int i = 0; i < fps * 3; i++)
        {
            vacuum.Update(1.0f / fps);
        }

        Check(std::abs(vacuum.GetPosition().y + .5f) < .0001f, "Vacuum sinks into Tiled floor");
        Check(vacuum.IsGround() && vacuum.landings == 1, "Grounded state/landing event");
        vacuum.Jump();
        vacuum.Update(1.0f / fps);
        Check(vacuum.GetPosition().y > -.5f, "Jump trapped on floor");

        for (int i = 0; i < fps * 2; i++)
        {
            vacuum.Update(1.0f / fps);
        }

        Check(vacuum.IsGround() && vacuum.landings == 2, "Landing from jump");
        vacuum.MoveX(31);
        vacuum.Update(1.0f / fps);
        Check(!vacuum.IsGround() && vacuum.GetPosition().y < -.5f, "Platform edge must fall");
        GroundProbe drop;
        drop.SetPosition({10, 10, 0});

        for (int i = 0; i < fps * 3; i++)
        {
            drop.Update(1.0f / fps);
        }

        Check(std::abs(drop.GetPosition().y + .5f) < .0001f && drop.landings == 1, "Drop-through floor");
        collision.RegisterTerrainAABB({{32, -2, -2}, {34, .3f, 2}});
        drop.MoveX(33);
        drop.Update(1.0f / fps);
        Check(drop.GetPosition().y < 0, "Normal motion snapped up a block");

        collision.RegisterTerrainAABB({{18.5f, -.5f, -2}, {19.5f, .5f, 2}});
        collision.RegisterTerrainAABB({{18.5f, .5f, -2}, {19.5f, 1.5f, 2}});
        collision.RegisterTerrainAABB({{18.5f, 5, -2}, {19.5f, 6, 2}});
        GroundProbe actual;
        actual.SetPosition({18.5625f, -1.0625f, 0});

        for (int i = 0; i < fps; i++)
        {
            actual.Update(1.0f / fps);
        }

        Check(std::abs(actual.GetPosition().y - 1.5f) < .0001f, "Actual column 19 placement/ceiling");
    }

    collision.ClearTerrainAABBs();
    collision.RegisterTerrainAABB({{0, -2, -1}, {5, 0, 1}});
    collision.RegisterTerrainAABB({{0, 4, -1}, {5, 5, 1}});
    float y = 0;
    Check(collision.FindTerrainFloor(2, 0, -1, 5, y) && y == 5, "Highest crossed floor selection");
    Check(!collision.FindTerrainFloor(2, 3, -1, 5, y), "Off-depth floor false hit");
    std::cout
        << "PASS: actual Vacuum spawn, 30/60/120 FPS standing/falling/jumping, edge fall, no step-up, landing callbacks\n";
}
