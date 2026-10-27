#pragma once
#include <cmath>

class EnemyActivation
{
  public:

    // 更新処理
    bool Update(float enemyX, float enemyY, float playerX, float playerY)
    {
        if (!active && std::abs(enemyX - playerX) <= 7.0f && std::abs(enemyY - playerY) <= 3.0f)
        {
            active = true;
        }

        return active;
    }

    bool IsActive() const
    {
        return active;
    }

    // UpdateVisibility の更新処理
    bool UpdateVisibility(bool visible)
    {
        if (visible)
        {
            active = true;
        }

        return active;
    }

  private:

    bool active = false;
};
