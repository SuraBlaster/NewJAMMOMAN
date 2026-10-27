#include "EnemyWalkerPatrol.h"
#include "EnemyActivation.h"
#include "Character.h"
#include "Model.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace DirectX;

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class PatrolProbe final : public Character
{
  public:

    PatrolProbe()
    {
        radius = 0.35f;
        height = 1.2f;
        useGravity = true;
    }

    void Update(float elapsedTime) override
    {
        float remainingTime = elapsedTime;

        while (remainingTime > 0.0f)
        {
            const float step = (std::min)(remainingTime, 1.0f / 60.0f);

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
            remainingTime -= step;
        }
    }

    float direction = -1.0f;
};

static void TestPatrol()
{
    auto& collision = CollisionManager::Instance();

    for (int fps : {10, 30, 60, 120})
    {
        collision.ClearTerrainAABBs();
        collision.RegisterTerrainAABB({{0, -1, -1}, {6, 0, 1}});
        collision.RegisterTerrainAABB({{4, 0, -1}, {5, 3, 1}});

        PatrolProbe enemy;
        enemy.SetPosition({2, 0, 0});

        bool movedLeft = false;
        bool movedRight = false;

        for (int frame = 0; frame < fps * 20; ++frame)
        {
            enemy.Update(1.0f / fps);

            Check(enemy.GetPosition().x >= 0.35f, "Walker fell off left ledge");
            Check(enemy.GetPosition().x <= 3.651f, "Walker crossed wall");
            Check(std::abs(enemy.GetPosition().y) < 0.001f, "Walker lost floor contact");

            movedLeft |= enemy.direction < 0.0f;
            movedRight |= enemy.direction > 0.0f;
        }

        Check(movedLeft && movedRight, "Walker did not patrol both directions");
    }

    collision.ClearTerrainAABBs();
    collision.RegisterTerrainAABB({{0, -1, -1}, {3, 0, 1}});

    Check(EnemyWalkerPatrol::ShouldTurn({2.7f, 0, 0}, 0.35f, 1.2f, 1, 0.02f), "Right ledge missed");
    Check(!EnemyWalkerPatrol::ShouldTurn({1, 2, 0}, 0.35f, 1.2f, 1, 0.02f), "Falling enemy turns in midair");

    EnemyActivation activation;
    Check(!activation.Update(10, 0, 0, 0), "Distant enemy activated");
    Check(activation.Update(6, 0, 0, 0), "Nearby enemy did not activate");
    Check(activation.Update(20, 0, 0, 0), "Activation did not latch");

    collision.ClearTerrainAABBs();
}

static void TestModel(ID3D11Device* device)
{
    Model model(device, "Data/Model/Enemy/Walker/robot.gltf", 24.0f);

    Check(model.GetAnimationIndex("iddle") >= 0, "Missing idle animation");
    Check(model.GetAnimationIndex("walking") >= 0, "Missing walking animation");
    Check(!model.GetMeshes().empty(), "Missing mesh");
    Check(model.GetMaterials().at(0).baseMap != nullptr, "Missing base texture");
    Check(model.GetMaterials().at(0).normalMap != nullptr, "Missing normal texture");

    for (const char* clipName : {"iddle", "walking"})
    {
        const int clip = model.GetAnimationIndex(clipName);

        for (int sample = 0; sample <= 8; ++sample)
        {
            std::vector<Model::NodePose> poses;
            model.ComputeAnimation(clip, model.GetAnimations()[clip].secondsLength * sample / 8, poses);
            model.SetNodePoses(poses);

            XMFLOAT4X4 transform;
            XMStoreFloat4x4(&transform, XMMatrixScaling(0.15f, 0.15f, 0.15f));
            model.UpdateTransform(transform);

            float minimumY = 1000.0f;
            float maximumY = -1000.0f;

            for (const auto& mesh : model.GetMeshes())
            {
                for (const auto& vertex : mesh.vertices)
                {
                    const float weights[] = {vertex.boneWeight.x, vertex.boneWeight.y, vertex.boneWeight.z,
                                             vertex.boneWeight.w};
                    const unsigned int indices[] = {vertex.boneIndex.x, vertex.boneIndex.y,
                                                    vertex.boneIndex.z, vertex.boneIndex.w};
                    XMVECTOR point = XMVectorZero();

                    for (int index = 0; index < 4; ++index)
                    {
                        if (weights[index] > 0.0f)
                        {
                            const auto& bone = mesh.bones.at(indices[index]);
                            const XMMATRIX boneTransform = XMLoadFloat4x4(&bone.offsetTransform) *
                                                           XMLoadFloat4x4(&bone.node->worldTransform);
                            point = XMVectorAdd(
                                point, XMVectorScale(XMVector3TransformCoord(XMLoadFloat3(&vertex.position),
                                                                             boneTransform),
                                                     weights[index]));
                        }
                    }

                    const float y = XMVectorGetY(point);

                    Check(std::isfinite(y), "Invalid skinned vertex");
                    minimumY = (std::min)(minimumY, y);
                    maximumY = (std::max)(maximumY, y);
                }
            }

            std::cout << clipName << " sample " << sample << ": y=" << minimumY << ".." << maximumY << '\n';
            Check(minimumY > -0.15f && minimumY < 0.3f, "Feet do not match ground");
            Check(maximumY > 0.7f && maximumY < 1.6f, "Unexpected model height");
        }
    }
}

int main()
{
    try
    {
        std::cout << "Creating WARP device" << std::endl;
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        Microsoft::WRL::ComPtr<ID3D11Device> device;
        const HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                                 D3D11_SDK_VERSION, &device, nullptr, nullptr);

        Check(SUCCEEDED(result), "Cannot create test device");
        std::cout << "Testing patrol" << std::endl;
        TestPatrol();
        std::cout << "Testing model" << std::endl;
        TestModel(device.Get());

        std::cout
            << "PASS: wall/ledge patrol at 10/30/60/120 FPS, activation, glTF textures and skinned animation bounds\n";
        CoUninitialize();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
