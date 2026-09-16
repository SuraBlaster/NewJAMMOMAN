#pragma once

#include <memory>
#include <vector>

#include "Enemy.h"

class ModelRenderer;
class ShapeRenderer;

class EnemyManager
{
public:
	static EnemyManager& Instance()
	{
		static EnemyManager instance;
		return instance;
	}

	EnemyManager(const EnemyManager&) = delete;
	EnemyManager& operator=(const EnemyManager&) = delete;

	void Update(float elapsedTime);
	void Render(ModelRenderer* modelRenderer) const;
	void DrawPrimitive(ShapeRenderer* shapeRenderer) const;
	void DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const;

	void Register(const std::shared_ptr<Enemy>& enemy);
	void Remove(const std::shared_ptr<Enemy>& enemy);
	void Clear();

	size_t GetEnemyCount() const { return enemies.size(); }
	const std::shared_ptr<Enemy>& GetEnemy(size_t index) const { return enemies.at(index); }

	template<typename T>
	std::shared_ptr<T> GetEnemyByType() const
	{
		for (const auto& enemy : enemies)
		{
			if (auto casted = std::dynamic_pointer_cast<T>(enemy)) return casted;
		}
		return nullptr;
	}

	template<typename T>
	std::vector<std::shared_ptr<T>> GetEnemiesByType() const
	{
		std::vector<std::shared_ptr<T>> result;
		for (const auto& enemy : enemies)
		{
			if (auto casted = std::dynamic_pointer_cast<T>(enemy)) result.push_back(casted);
		}
		return result;
	}

private:
	EnemyManager() = default;
	~EnemyManager() = default;

	std::vector<std::shared_ptr<Enemy>> enemies;
	std::vector<std::shared_ptr<Enemy>> pending_enemies;
};
