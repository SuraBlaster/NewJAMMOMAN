#include "EnemyFactory.h"
#include "Player.h"
#include "EnemyBoss.h"
#include "EnemyEgg.h"
#include "EnemyFan.h"
#include "EnemyFly.h"
#include "EnemyHover.h"
#include "EnemyMage.h"
#include "EnemyScatter.h"
#include "EnemyWave.h"
#include "EnemyVacuum.h"
#include "EnemyWalker.h"

std::shared_ptr<Enemy> EnemyFactory::CreateFromSpawnData(ID3D11Device* device, Player* player,
                                                         const EnemySpawnData& spawnData)
{
    // 2.5Dの奥行きレーンを揃えるため、Z座標はプレイヤーの現在位置を使用する
    DirectX::XMFLOAT3 spawnPosition = {spawnData.positionX, spawnData.positionY, player->GetPosition().z};

    // 共通のEnemyポインタとして返し、呼び出し側の登録処理を敵種別から独立させる
    switch (spawnData.enemyType)
    {
    case EnemyType::Wave:
    {
        auto enemyWave = std::make_shared<EnemyWave>(device, player);
        enemyWave->SetPosition(spawnPosition);
        // StageData側の方向をEnemyWave固有の方向型へ変換する
        if (spawnData.enemySpawnDirection == EnemySpawnDirection::Right)
        {
            enemyWave->SetDirection(EnemyWave::Direction::Right);
        }
        else
        {
            enemyWave->SetDirection(EnemyWave::Direction::Left);
        }

        return enemyWave;
    }

    case EnemyType::Scatter:
    {
        auto enemyScatter = std::make_shared<EnemyScatter>(device, player, spawnPosition);
        return enemyScatter;
    }

    case EnemyType::Egg:
    {
        auto enemyEgg = std::make_shared<EnemyEgg>(device, player, spawnPosition);
        return enemyEgg;
    }

    case EnemyType::Mage:
    {
        auto enemyMage = std::make_shared<EnemyMage>(device, player);
        enemyMage->SetPosition(spawnPosition);
        enemyMage->SetDirection(spawnData.enemySpawnDirection == EnemySpawnDirection::Right ? 1.0f : -1.0f);
        return enemyMage;
    }

    case EnemyType::Fan:
    {
        auto enemyFan = std::make_shared<EnemyFan>(device, player);
        enemyFan->SetPosition(spawnPosition);
        return enemyFan;
    }

    case EnemyType::Hover:
    {
        auto enemyHover = std::make_shared<EnemyHover>(device, player);
        enemyHover->SetPosition(spawnPosition);
        return enemyHover;
    }

    case EnemyType::Fly:
    {
        auto enemyFly = std::make_shared<EnemyFly>(device, player);
        enemyFly->SetPosition(spawnPosition);
        return enemyFly;
    }

    case EnemyType::Boss:
    {
        auto enemyBoss = std::make_shared<EnemyBoss>(device, player);
        enemyBoss->SetPosition(spawnPosition);
        return enemyBoss;
    }

    case EnemyType::Vacuum:
    {
        auto enemyVacuum = std::make_shared<EnemyVacuum>(device, player);
        enemyVacuum->SetPosition(spawnPosition);
        enemyVacuum->SetDirection(spawnData.enemySpawnDirection == EnemySpawnDirection::Right ? 1.0f : -1.0f);
        return enemyVacuum;
    }

    case EnemyType::Walker:
    {
        auto enemyWalker = std::make_shared<EnemyWalker>(device, player);
        enemyWalker->SetPosition(spawnPosition);
        enemyWalker->SetDirection(spawnData.enemySpawnDirection == EnemySpawnDirection::Right ? 1.0f : -1.0f);

        return enemyWalker;
    }

    case EnemyType::Unknown:
    {
    }

    default:
    {
    }
    }

    return nullptr;
}
