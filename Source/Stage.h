#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include "Model.h"
#include "StageData.h"
#include "StageObject.h"

class Stage
{
public:
	Stage(ID3D11Device* device, const StageData& stageData, const TileTypeManager& tileTypeManager);
	~Stage() = default;

	const std::vector<std::unique_ptr<StageObject>>& GetStageObjects() const { return stageObjects; }

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
