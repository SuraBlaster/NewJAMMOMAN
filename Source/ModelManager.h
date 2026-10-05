#pragma once
#include "Model.h"
#include <map>
#include <memory>
#include <string>

// Main-thread only. Keep prototypes across scene transitions, but never expose
// their mutable pose to a character. Clear after scenes are destroyed at shutdown.
class ModelManager
{
public:
    static ModelManager& Instance();
    std::shared_ptr<Model> CreateInstance(ID3D11Device* device,
        const char* filename, float sampleRate = 60.0f);
    void Clear();

private:
    ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;
    Microsoft::WRL::ComPtr<ID3D11Device> cachedDevice;
    std::map<std::pair<std::wstring, float>, std::shared_ptr<const Model>> prototypes;
};
