#include "Character.h"
#include "EnemyWalkerPatrol.h"
#include "Animator.h"

#include <json.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>

class WalkingProbe final : public Character
{
  public:

    WalkingProbe()
    {
        useGravity = true;
        height = 1.2f;
        radius = 0.35f;
    }

    void Update(float elapsedTime) override
    {
        float remaining = elapsedTime;

        while (remaining > 0.0f)
        {
            const float step = (std::min)(remaining, 1.0f / 60.0f);

            if (EnemyWalkerPatrol::ShouldTurn(position, radius, height, direction, direction * step))
            {
                direction = -direction;
                velocity.x = 0.0f;
            }
            else
            {
                velocity.x = direction;
            }

            UpdateGroundPhysics(step);
            remaining -= step;
        }
    }

    float direction = -1.0f;
};

template <class Function> static void Measure(const char* label, Function function)
{
    constexpr int FRAMES = 180;
    const auto begin = std::chrono::steady_clock::now();

    for (int frame = 0; frame < FRAMES; ++frame)
    {
        function();
    }

    const double milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    std::cout << label << ": " << milliseconds / FRAMES << " ms/frame" << std::endl;
}

int main()
{
    auto& collisions = CollisionManager::Instance();
    nlohmann::json stage;
    std::ifstream("Data/Stage/LaboratoryCombinedCourse.stage.json") >> stage;
    size_t count = 0;

    for (const auto& object : stage.at("objects"))
    {
        if (object.at("typeId") != 100)
        {
            continue;
        }

        const float x = object.at("position").at("x");
        const float y = object.at("position").at("y");
        collisions.RegisterTerrainAABB({{x - 0.5f, y - 0.5f, -0.5f}, {x + 0.5f, y + 0.5f, 0.5f}});
        ++count;
    }

    std::cout << "Terrain boxes: " << count << std::endl;
    std::vector<WalkingProbe> enemies(15);

    for (size_t index = 0; index < enemies.size(); ++index)
    {
        enemies[index].SetPosition({5.0f + float(index) * 0.5f, -0.5f, 0});
    }

    Measure("15 walkers physics at 60 FPS",
            [&]()
            {
                for (auto& enemy : enemies)
                {
                    enemy.Update(1.0f / 60.0f);
                }
            });

    Measure("15 walkers physics at 30 FPS",
            [&]()
            {
                for (auto& enemy : enemies)
                {
                    enemy.Update(1.0f / 30.0f);
                }
            });

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device,
                      nullptr, nullptr);

    const Model prototype(device.Get(), "Data/Model/Enemy/Walker/robot.gltf", 24.0f);
    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<Animator>> animators;

    for (int index = 0; index < 15; ++index)
    {
        models.push_back(std::make_unique<Model>(prototype));
        animators.push_back(std::make_unique<Animator>(models.back().get()));
        animators.back()->Play(0, prototype.GetAnimationIndex("walking"), true);
    }

    DirectX::XMFLOAT4X4 identity;
    DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());

    Measure("15 walkers animation and transforms",
            [&]()
            {
                for (size_t index = 0; index < models.size(); ++index)
                {
                    animators[index]->Update(1.0f / 60.0f);
                    models[index]->UpdateTransform(identity);
                }
            });

    collisions.ClearTerrainAABBs();
    CoUninitialize();
}
