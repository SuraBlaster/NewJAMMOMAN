#include "EnemyManager.h"

#include <algorithm>

void EnemyManager::Update(float elapsedTime)
{
	if (!pendingEnemies.empty())
	{
		enemies.insert(enemies.end(), pendingEnemies.begin(), pendingEnemies.end());
		pendingEnemies.clear();
	}

	for (const auto& enemy : enemies)
	{
		if (enemy && !enemy->IsDestroyRequested()) enemy->Update(elapsedTime);
	}

	enemies.erase(
		std::remove_if(enemies.begin(), enemies.end(),
			[](const std::shared_ptr<Enemy>& enemy)
			{
				return !enemy || enemy->IsDestroyRequested();
			}),
		enemies.end());
}

void EnemyManager::Render(ModelRenderer* modelRenderer) const
{
	for (const auto& enemy : enemies)
	{
		if (enemy && !enemy->IsDestroyRequested()) enemy->Render(modelRenderer);
	}
}

void EnemyManager::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
	for (const auto& enemy : enemies)
	{
		if (enemy && !enemy->IsDestroyRequested()) enemy->DrawDebugPrimitive(shapeRenderer);
	}
}

void EnemyManager::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
	for (const auto& enemy : enemies)
	{
		if (enemy && !enemy->IsDestroyRequested()) enemy->DrawPrimitive(shapeRenderer);
	}
}

void EnemyManager::Register(const std::shared_ptr<Enemy>& enemy)
{
	if (enemy) pendingEnemies.push_back(enemy);
}

void EnemyManager::Remove(const std::shared_ptr<Enemy>& enemy)
{
	if (enemy) enemy->Destroy();
}

void EnemyManager::Clear()
{
	enemies.clear();
	pendingEnemies.clear();
}
