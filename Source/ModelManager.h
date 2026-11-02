#pragma once
#include <memory>
#include <utility>
#include <map>
#include <string>
#include "Model.h"

class ModelManager
{
public:
    static ModelManager& Instance();
    std::shared_ptr<Model> CreateInstance(ID3D11Device* device, const char* filename,
                                          float sampleRate = 60.0f);
    void Clear();

private:
    // コンストラクタ
    ModelManager() = default;

    // コンストラクタ
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;
    Microsoft::WRL::ComPtr<ID3D11Device> cachedDevice;
    std::map<std::pair<std::wstring, float>, std::shared_ptr<const Model>> prototypes;
};
