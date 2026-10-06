#include "LoadingProfile.h"
#include "Stage.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace
{
    bool TryMergeTerrainBoxes(
        const AABB& a,
        const AABB& b,
        int mergeAxis,
        AABB& merged)
    {
        // 座標変換による微小な誤差だけを許容する。
        constexpr float tolerance = 0.00001f;

        const float aMin[3] = {
            a.min.x, a.min.y, a.min.z
        };

        const float aMax[3] = {
            a.max.x, a.max.y, a.max.z
        };

        const float bMin[3] = {
            b.min.x, b.min.y, b.min.z
        };

        const float bMax[3] = {
            b.max.x, b.max.y, b.max.z
        };

        for (int axis = 0; axis < 3; ++axis)
        {
            if (axis == mergeAxis)
            {
                // 統合方向に隙間があれば、まとめない。
                if (aMax[axis] < bMin[axis] - tolerance ||
                    bMax[axis] < aMin[axis] - tolerance)
                {
                    return false;
                }
            }
            else
            {
                // 他の2軸は、範囲が一致する必要がある。
                if (std::abs(aMin[axis] - bMin[axis])
                    > tolerance ||
                    std::abs(aMax[axis] - bMax[axis])
                > tolerance)
                {
                    return false;
                }
            }
        }

        merged.min = {
            (std::min)(a.min.x, b.min.x),
            (std::min)(a.min.y, b.min.y),
            (std::min)(a.min.z, b.min.z)
        };

        merged.max = {
            (std::max)(a.max.x, b.max.x),
            (std::max)(a.max.y, b.max.y),
            (std::max)(a.max.z, b.max.z)
        };

        return true;
    }
}

Stage::Stage(ID3D11Device* device, const StageData& stageData, const TileTypeManager& tileTypeManager)
{
	LoadingProfile::Scope profile("Stage");
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

        std::shared_ptr<Model> collisionModel;
        if (tileInfo->collision && !tileInfo->collisionModelPath.empty())
        {
            auto it = modelPrototypeCache.find(tileInfo->collisionModelPath);
            if (it == modelPrototypeCache.end())
            {
                auto prototype = std::make_shared<Model>(device, tileInfo->collisionModelPath.c_str());
                it = modelPrototypeCache.emplace(tileInfo->collisionModelPath, prototype).first;
            }
            collisionModel = std::make_shared<Model>(*it->second);
        }

		stageObjects.push_back(
			std::make_unique<StageObject>(
				instanceModel,
				objectData,
				*tileInfo,
				fixedPositionZ, collisionModel));
	}

    // 全ブロックの生成後に、衝突用の箱をまとめる。
    profile.Step("objects_including_model_and_aabbs");
    BuildTerrainAABBs();
    profile.Step("merge_terrain_aabbs");
}

void Stage::BuildTerrainAABBs()
{
    terrainBoxes.clear();

    // 1. 衝突が有効なオブジェクトの箱を集める。
    for (const auto& object : stageObjects)
    {
        AABB box;

        if (object->GetCollisionAABB(box))
        {
            terrainBoxes.push_back(box);
        }
    }

    // 2. 縦、横、奥行きの順で統合を試す。
    const int axisOrder[3] = { 1, 0, 2 };

    // Group equal cross-sections and merge consecutive intervals in one pass.
    // Sorting uses exact comparisons (a strict ordering); tolerance is applied
    // only by TryMergeTerrainBoxes. Unmerged near-equal boxes remain valid.
    bool changed;
    std::vector<AABB> compact;
    compact.reserve(terrainBoxes.size());
    do
    {
        changed = false;
        for (int axis : axisOrder)
        {
            const auto component = [](const DirectX::XMFLOAT3& v, int a) {
                return a == 0 ? v.x : a == 1 ? v.y : v.z;
            };
            std::sort(terrainBoxes.begin(), terrainBoxes.end(), [&](const AABB& a, const AABB& b) {
                for (int offset = 1; offset <= 2; ++offset)
                {
                    const int cross = (axis + offset) % 3;
                    const float amin = component(a.min, cross), bmin = component(b.min, cross);
                    if (amin != bmin) return amin < bmin;
                    const float amax = component(a.max, cross), bmax = component(b.max, cross);
                    if (amax != bmax) return amax < bmax;
                }
                const float amin = component(a.min, axis), bmin = component(b.min, axis);
                if (amin != bmin) return amin < bmin;
                return component(a.max, axis) < component(b.max, axis);
            });
            compact.clear();
            for (const auto& box : terrainBoxes)
            {
                AABB merged;
                if (!compact.empty() && TryMergeTerrainBoxes(compact.back(), box, axis, merged))
                {
                    compact.back() = merged;
                    changed = true;
                }
                else compact.push_back(box);
            }
            terrainBoxes.swap(compact);
        }
    } while (changed);
}