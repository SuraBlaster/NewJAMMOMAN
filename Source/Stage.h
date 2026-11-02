#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "StageData.h"
#include "StageObject.h"
#include "Model.h"

class Stage
{
public:
    // コンストラクタ
    Stage(ID3D11Device* device, const StageData& stageData, const TileTypeManager& tileTypeManager);

    // デストラクタ
    ~Stage() = default;

    const std::vector<std::unique_ptr<StageObject>>& GetStageObjects() const
    {
        return stageObjects;
    }

    const std::vector<AABB>& GetTerrainAABBs() const
    {
        return terrainBoxes;
    }

private:
    void BuildTerrainAABBs();

private:
    std::unordered_map<std::string, std::shared_ptr<Model>> modelPrototypeCache;

    std::vector<std::unique_ptr<StageObject>> stageObjects;

    std::vector<AABB> terrainBoxes;
};
