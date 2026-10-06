#include "ModelManager.h"
#include "LoadingProfile.h"
#include <filesystem>
#include <stdexcept>
#include <cmath>

ModelManager& ModelManager::Instance()
{
    static ModelManager instance;
    return instance;
}

std::shared_ptr<Model> ModelManager::CreateInstance(
    ID3D11Device* device, const char* filename, float sampleRate)
{
    if (!device || !filename || !*filename || !std::isfinite(sampleRate) || sampleRate <= 0)
        throw std::invalid_argument("Invalid model instance arguments");

    LoadingProfile::Scope profile(std::string("ModelManager:") + filename);
    // Absolute normalized paths make Data/... and ./Data/... share one entry.
    const auto path = std::filesystem::weakly_canonical(std::filesystem::absolute(filename));
    const auto key = std::make_pair(path.wstring(), sampleRate);
    if (cachedDevice.Get() != device)
    {
        Clear();
        cachedDevice = device;
    }
    auto it = prototypes.find(key);
    if (it == prototypes.end())
    {
        auto prototype = std::make_shared<Model>(device, filename, sampleRate);
        it = prototypes.emplace(key, std::move(prototype)).first;
        profile.Step("load_prototype");
    }
    else
    {
        profile.Step("cache_hit");
    }
    // Model's copy constructor fixes node/material/bone pointers and shares
    // ComPtr GPU resources, while retaining independent CPU pose data.
    auto instance = std::make_shared<Model>(*it->second);
    profile.Step("copy_instance");
    return instance;
}

void ModelManager::Clear()
{
    prototypes.clear();
    cachedDevice.Reset();
}
