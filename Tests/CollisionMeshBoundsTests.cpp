#include "CollisionManager.h"
#include "Model.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

using namespace DirectX;

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                      D3D11_SDK_VERSION, &device, nullptr, nullptr)),
          "Test device failed");

    Model model(device.Get(), "Data/Model/Cube/Cube.gltf");
    XMFLOAT4X4 identity;
    XMStoreFloat4x4(&identity, XMMatrixIdentity());
    model.UpdateTransform(identity);

    std::vector<XMFLOAT4X4> transforms(24);
    std::vector<AABB> bounds(24);
    std::vector<CollisionMesh> meshes(24);

    for (size_t index = 0; index < meshes.size(); ++index)
    {
        const auto transform = XMMatrixScaling(1.0f, 0.5f + index * 0.1f, 1.0f) *
                               XMMatrixRotationZ(float(index) * 0.17f) *
                               XMMatrixTranslation(float(index % 8) * 2 - 8, float(index / 8) * 3, 0);
        XMStoreFloat4x4(&transforms[index], transform);

        const float limit = (std::numeric_limits<float>::max)();
        bounds[index] = {{limit, limit, limit}, {-limit, -limit, -limit}};

        for (const auto& mesh : model.GetMeshes())
        {
            for (const auto& vertex : mesh.vertices)
            {
                XMFLOAT3 point;
                XMStoreFloat3(
                    &point, XMVector3TransformCoord(XMLoadFloat3(&vertex.position),
                                                    XMLoadFloat4x4(&mesh.node->globalTransform) * transform));

                bounds[index].min = {(std::min)(bounds[index].min.x, point.x),
                                     (std::min)(bounds[index].min.y, point.y),
                                     (std::min)(bounds[index].min.z, point.z)};
                bounds[index].max = {(std::max)(bounds[index].max.x, point.x),
                                     (std::max)(bounds[index].max.y, point.y),
                                     (std::max)(bounds[index].max.z, point.z)};
            }
        }

        meshes[index].model = &model;
        meshes[index].transform = &transforms[index];
        CollisionManager::Instance().Register(&meshes[index]);
    }

    std::mt19937 random(9813);
    std::uniform_real_distribution<float> horizontal(-12, 12);
    std::uniform_real_distribution<float> vertical(-3, 12);
    std::uniform_real_distribution<float> depth(-2, 2);
    size_t hits = 0;

    for (int ray = 0; ray < 2000; ++ray)
    {
        const XMFLOAT3 start = {horizontal(random), vertical(random), depth(random)};
        const XMFLOAT3 end = {horizontal(random), vertical(random), depth(random)};

        for (auto& mesh : meshes)
        {
            mesh.bounds = nullptr;
        }

        HitResult reference;
        const bool referenceHit = CollisionManager::Instance().Raycast(start, end, reference);

        for (size_t index = 0; index < meshes.size(); ++index)
        {
            meshes[index].bounds = &bounds[index];
        }

        HitResult actual;
        const bool actualHit = CollisionManager::Instance().Raycast(start, end, actual);
        Check(referenceHit == actualHit, "Bounded ray changed hit result");

        if (referenceHit)
        {
            ++hits;
            Check(std::abs(reference.distance - actual.distance) < 0.0001f,
                  "Bounded ray changed closest hit");
            Check(std::abs(reference.position.y - actual.position.y) < 0.0001f,
                  "Bounded ray changed floor height");
        }
    }

    HitResult unused;
    Check(!CollisionManager::Instance().Raycast({50, 0, 0}, {50, 0, 0}, unused), "Zero-length ray hit");

    for (auto& mesh : meshes)
    {
        CollisionManager::Instance().Unregister(&mesh);
    }

    Check(hits > 100, "Insufficient positive test coverage");
    std::cout << "PASS: 2000 bounded/unbounded ray comparisons, " << hits
              << " hits, rotated/scaled meshes, nearest hit, zero-length ray\n";
    CoUninitialize();
}
