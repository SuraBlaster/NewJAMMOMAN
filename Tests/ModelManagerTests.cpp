#include "ModelManager.h"
#include <stdexcept>
#include <iostream>

static void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com)) return 1;
    try
    {
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP,
            nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr)), "device");
        auto& manager = ModelManager::Instance();
        auto a = manager.CreateInstance(device.Get(), "Data/Model/Jammo/Jammo_Player.gltf");
        auto b = manager.CreateInstance(device.Get(), "./Data/Model/Jammo/Jammo_Player.gltf");
        Check(a != b, "instances must differ");
        Check(!a->GetMeshes().empty(), "meshes missing");
        Check(a->GetMeshes()[0].vertexBuffer == b->GetMeshes()[0].vertexBuffer, "GPU buffers not shared");
        Check(a->GetMeshes()[0].material->baseMap == b->GetMeshes()[0].material->baseMap, "textures not shared");
        Check(a->GetMeshes()[0].node != b->GetMeshes()[0].node, "node alias");
        const float original = b->GetNodes()[0].position.x;
        a->GetNodes()[0].position.x += 17.0f;
        Check(b->GetNodes()[0].position.x == original, "pose leaked");
        a->GetMeshes()[0].material->baseMap.Reset();
        Check(b->GetMeshes()[0].material->baseMap.Get() != nullptr, "material change leaked");
        for (const auto& mesh : b->GetMeshes())
        {
            Check(mesh.node == &b->GetNodes().at(mesh.nodeIndex), "mesh node not rebound");
            Check(mesh.material == &b->GetMaterials().at(mesh.materialIndex), "material not rebound");
            for (const auto& bone : mesh.bones)
                Check(bone.node == &b->GetNodes().at(bone.nodeIndex), "bone not rebound");
        }
        auto buffer = b->GetMeshes()[0].vertexBuffer;
        a.reset();
        b.reset();
        auto c = manager.CreateInstance(device.Get(), "Data/Model/Jammo/Jammo_Player.gltf");
        Check(c->GetMeshes()[0].vertexBuffer == buffer, "cache lost between scenes");
        Check(c->GetNodes()[0].position.x == original, "prototype was mutated");
        Check(c->GetMeshes()[0].material->baseMap.Get() != nullptr, "prototype material was mutated");
        manager.Clear();
        Check(c->GetMeshes()[0].vertexBuffer == buffer, "Clear invalidated live instance");
        auto d = manager.CreateInstance(device.Get(), "Data/Model/Jammo/Jammo_Player.gltf");
        Check(d->GetMeshes()[0].vertexBuffer != buffer, "Clear did not evict prototype");
        auto cube60 = manager.CreateInstance(device.Get(), "Data/Model/Cube/Cube.gltf", 60);
        auto cube30 = manager.CreateInstance(device.Get(), "Data/Model/Cube/Cube.gltf", 30);
        Check(cube60->GetMeshes()[0].vertexBuffer != cube30->GetMeshes()[0].vertexBuffer, "sample rates share cache key");
        manager.Clear();
        std::cout << "ModelManager tests passed\n";
        // Repeated stage enemies share GPU resources but retain independent poses.
        for (const char* path : {"Data/Model/Enemy/Wizard.gltf", "Data/Model/Enemy/FlyEnemy.gltf"})
        {
            auto first = manager.CreateInstance(device.Get(), path);
            auto second = manager.CreateInstance(device.Get(), path);
            Check(first != second, "enemy instances alias");
            Check(first->GetMeshes()[0].vertexBuffer == second->GetMeshes()[0].vertexBuffer, "enemy GPU resource duplication");
            const auto saved = second->GetNodes()[0].position.x;
            first->GetNodes()[0].position.x += 10;
            Check(second->GetNodes()[0].position.x == saved, "enemy pose leaked");
            for (const auto& mesh : second->GetMeshes())
                for (const auto& bone : mesh.bones)
                    Check(bone.node == &second->GetNodes().at(bone.nodeIndex), "enemy skeleton not rebound");
        }
        manager.Clear();
        std::cout << "Enemy instance sharing tests passed\n";
    }
    catch (const std::exception& e)
    {
        ModelManager::Instance().Clear();
        std::cerr << e.what() << '\n';
        CoUninitialize();
        return 1;
    }
    CoUninitialize();
    return 0;
}
