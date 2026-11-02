#include "Model.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

// 変換と読み戻し検証
int main()
{
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com))
    {
        return 1;
    }
    int result = 0;
    try
    {
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                     &device, nullptr, nullptr)))
        {
            throw std::runtime_error("Cannot create verification device");
        }
        std::ifstream manifest("Tools/ModelCerealSources.txt");
        if (!manifest)
        {
            throw std::runtime_error("Cannot read source manifest");
        }
        std::string source;
        int count = 0;
        while (std::getline(manifest, source))
        {
            if (!source.empty() && source.back() == '\r')
            {
                source.pop_back();
            }
            if (source.empty())
            {
                continue;
            }
            const float sampleRate = source.find("/Walker/robot.") != std::string::npos ? 24.0f : 60.0f;
            Model::ConvertToCereal(source.c_str(), sampleRate);
            std::filesystem::path output(source);
            output.replace_extension(".cereal");
            Model original(device.Get(), source.c_str(), sampleRate);
            Model converted(device.Get(), output.string().c_str(), sampleRate);
            if (original.GetNodes().size() != converted.GetNodes().size() ||
                original.GetMeshes().size() != converted.GetMeshes().size() ||
                original.GetMaterials().size() != converted.GetMaterials().size() ||
                original.GetAnimations().size() != converted.GetAnimations().size())
            {
                throw std::runtime_error("Model structure changed: " + source);
            }
            for (size_t i = 0; i < original.GetMeshes().size(); ++i)
            {
                const auto& a = original.GetMeshes()[i];
                const auto& b = converted.GetMeshes()[i];
                if (a.vertices.size() != b.vertices.size() || a.indices != b.indices ||
                    a.bones.size() != b.bones.size() || !b.vertexBuffer || !b.indexBuffer)
                {
                    throw std::runtime_error("Mesh mismatch: " + source);
                }
            }
            for (const auto& material : converted.GetMaterials())
            {
                for (const auto& texture :
                     {material.baseTextureFileName, material.normalTextureFileName,
                      material.emissiveTextureFileName, material.occlusionTextureFileName,
                      material.metalnessRoughnessTextureFileName})
                {
                    if (!texture.empty() && !std::filesystem::is_regular_file(output.parent_path() / texture))
                    {
                        throw std::runtime_error("Missing texture: " + texture);
                    }
                }
            }
            for (size_t i = 0; i < original.GetAnimations().size(); ++i)
            {
                const auto& a = original.GetAnimations()[i];
                const auto& b = converted.GetAnimations()[i];
                if (a.name != b.name || a.secondsLength != b.secondsLength ||
                    a.nodeAnims.size() != b.nodeAnims.size())
                {
                    throw std::runtime_error("Animation mismatch: " + source);
                }
                for (size_t j = 0; j < a.nodeAnims.size(); ++j)
                {
                    if (a.nodeAnims[j].positionKeyframes.size() != b.nodeAnims[j].positionKeyframes.size() ||
                        a.nodeAnims[j].rotationKeyframes.size() != b.nodeAnims[j].rotationKeyframes.size() ||
                        a.nodeAnims[j].scaleKeyframes.size() != b.nodeAnims[j].scaleKeyframes.size())
                    {
                        throw std::runtime_error("Animation sampling mismatch: " + source);
                    }
                }
            }
            std::cout << "Verified: " << output.string() << std::endl;
            ++count;
        }
        std::cout << count << " models converted and verified." << std::endl;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        result = 1;
    }
    CoUninitialize();
    return result;
}