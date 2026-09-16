#include "Stage.h"
#include <stdexcept>

Stage::Stage(ID3D11Device* device, const StageData& stageData, const TileTypeManager& tileTypeManager)
{
	constexpr float fixedPositionZ = 0.0f;

	stageObjects.reserve(stageData.objects.size());

	for (const auto& objectData : stageData.objects)
	{
		const TileInfo* tileInfo = tileTypeManager.GetTileInfo(objectData.typeId);

		if (tileInfo == nullptr)
		{
			throw std::runtime_error("Failed to load TileInfo.");
		}

		auto cacheIt = modelPrototypeCache.find(tileInfo->modelPath);

		std::shared_ptr<Model> prototypeModel;

		if (cacheIt == modelPrototypeCache.end())
		{
			// キャッシュにないので、GLBから原本を読み込む
			prototypeModel = std::make_shared<Model>(device, tileInfo->modelPath.c_str());

			// 次回から再利用できるように原本を保存する
			modelPrototypeCache.emplace(tileInfo->modelPath, prototypeModel);
		}
		else
		{
			// キャッシュにある原本を取得する
			prototypeModel = cacheIt->second;
		}

		std::shared_ptr<Model> instanceModel = std::make_shared<Model>(*prototypeModel);

		stageObjects.push_back(
			std::make_unique<StageObject>(
				instanceModel,
				objectData,
				*tileInfo,
				fixedPositionZ));
	}
}
