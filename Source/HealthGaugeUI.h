#pragma once
#include <memory>
#include <d3d11.h>
#include <wrl.h>
#include "Sprite.h"
#include "HealthGaugeTrail.h"

class EnemyBoss;
class Player;

class HealthGaugeUI
{
  public:

    // コンストラクタ
    explicit HealthGaugeUI(ID3D11Device* device);
    void ShowBoss(const std::shared_ptr<EnemyBoss>& boss);

    // 更新処理
    void Update(const Player& player, float elapsedTime);
    void EnqueueRender();
    bool IsBossIntroPlaying() const;

  private:

    // 描画処理
    void Render();
    std::unique_ptr<Sprite> playerGauge;
    std::unique_ptr<Sprite> bossGauge;
    Microsoft::WRL::ComPtr<ID3D11Buffer> gaugeConstants;
    std::weak_ptr<EnemyBoss> encounterBoss;
    HealthGaugeTrail playerHealthTrail;
    HealthGaugeTrail bossHealthTrail;
    bool bossGaugeVisible = false;
    float bossGaugeIntroTime = 0.0f;
    static constexpr float BOSS_GAUGE_INTRO_DURATION = 1.5f;
};
