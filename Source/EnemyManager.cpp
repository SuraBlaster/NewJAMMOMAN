#include "EnemyManager.h"

#include <algorithm>
#include "Camera.h"
#include "ViewVolume.h"

namespace
{
    bool Visible(const Enemy& enemy, const ViewVolume& volume)
    {
        const auto& p = enemy.GetPosition();

        const float horizontal = (std::max)(4.0f, enemy.GetRadius() * 3.0f);
        const float vertical = (std::max)(4.0f, enemy.GetHeight() * 2.0f);
        return volume.Intersects(DirectX::BoundingBox({p.x, p.y + enemy.GetHeight() * 0.5f, p.z},
                                                      {horizontal, vertical, horizontal}));
    }
}

void EnemyManager::FlushPendingEnemies()
{
    enemies.insert(enemies.end(), pendingEnemies.begin(), pendingEnemies.end());
    pendingEnemies.clear();
}

// 更新処理
void EnemyManager::Update(float elapsedTime)
{
    FlushPendingEnemies();

    for (const auto& enemy : enemies)
    {
        if (enemy && !enemy->IsDestroyRequested())
        {
            enemy->Update(elapsedTime);
        }
    }

    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const std::shared_ptr<Enemy>& enemy)
                                 { return !enemy || enemy->IsDestroyRequested(); }),
                  enemies.end());
}

// 描画処理
void EnemyManager::Render(ModelRenderer* modelRenderer) const
{
    const auto& camera = Camera::Instance();
    const ViewVolume volume(camera.GetView(), camera.GetProjection());

    for (const auto& enemy : enemies)
    {
        if (enemy && !enemy->IsDestroyRequested() && Visible(*enemy, volume))
        {
            enemy->Render(modelRenderer);
        }
    }
}

// 当たり判定などのデバッグ描画処理
void EnemyManager::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
    const auto& camera = Camera::Instance();
    const ViewVolume volume(camera.GetView(), camera.GetProjection());

    for (const auto& enemy : enemies)
    {
        if (enemy && !enemy->IsDestroyRequested() && Visible(*enemy, volume))
        {
            enemy->DrawDebugPrimitive(shapeRenderer);
        }
    }
}

// DrawPrimitive の描画処理
void EnemyManager::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
    for (const auto& enemy : enemies)
    {
        if (enemy && !enemy->IsDestroyRequested())
        {
            enemy->DrawPrimitive(shapeRenderer);
        }
    }
}

void EnemyManager::Register(const std::shared_ptr<Enemy>& enemy)
{
    if (enemy)
    {
        pendingEnemies.push_back(enemy);
    }
}

void EnemyManager::Remove(const std::shared_ptr<Enemy>& enemy)
{
    if (enemy)
    {
        enemy->Destroy();
    }
}

void EnemyManager::Clear()
{
    enemies.clear();
    pendingEnemies.clear();
}
