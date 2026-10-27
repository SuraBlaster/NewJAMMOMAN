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

    // コンストラクタ
    EnemyManager(const EnemyManager&) = delete;
    EnemyManager& operator=(const EnemyManager&) = delete;

    // 更新処理
    void Update(float elapsedTime);
    void FlushPendingEnemies();

    // 描画処理
    void Render(ModelRenderer* modelRenderer) const;

    // DrawPrimitive の描画処理
    void DrawPrimitive(ShapeRenderer* shapeRenderer) const;

    // 当たり判定などのデバッグ描画処理
    void DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const;

    void Register(const std::shared_ptr<Enemy>& enemy);
    void Remove(const std::shared_ptr<Enemy>& enemy);
    void Clear();

    size_t GetEnemyCount() const
    {
        return enemies.size();
    }

    const std::shared_ptr<Enemy>& GetEnemy(size_t index) const
    {
        return enemies.at(index);
    }

    template <typename T> std::shared_ptr<T> GetEnemyByType() const
    {
        for (const auto& enemy : enemies)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(enemy))
            {
                return casted;
            }
        }

        return nullptr;
    }

    template <typename T> std::vector<std::shared_ptr<T>> GetEnemiesByType() const
    {
        std::vector<std::shared_ptr<T>> result;

        for (const auto& enemy : enemies)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(enemy))
            {
                result.push_back(casted);
            }
        }

        return result;
    }

  private:

    // コンストラクタ
    EnemyManager() = default;

    // デストラクタ
    ~EnemyManager() = default;

    std::vector<std::shared_ptr<Enemy>> enemies;
    std::vector<std::shared_ptr<Enemy>> pendingEnemies;
};
