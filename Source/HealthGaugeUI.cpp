#include "HealthGaugeUI.h"
#include <algorithm>
#include <stdexcept>
#include <imgui.h>
#include "Graphics.h"
#include "Player.h"
#include "EnemyBoss.h"

// コンストラクタ
HealthGaugeUI::HealthGaugeUI(ID3D11Device* device)
{
    playerGauge = std::make_unique<Sprite>(device, "Data/Sprite/PlayerGauge.png", true,
                                           "Data/Shader/HealthGaugePS.cso");
    bossGauge =
        std::make_unique<Sprite>(device, "Data/Sprite/EnemyGauge.png", true, "Data/Shader/HealthGaugePS.cso");
    D3D11_BUFFER_DESC gaugeBuffer = {};
    gaugeBuffer.ByteWidth = sizeof(DirectX::XMFLOAT4) * 3;
    gaugeBuffer.Usage = D3D11_USAGE_DEFAULT;
    gaugeBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (FAILED(device->CreateBuffer(&gaugeBuffer, nullptr, gaugeConstants.GetAddressOf())))
    {
        throw std::runtime_error("Failed to create health gauge constants.");
    }
}

void HealthGaugeUI::ShowBoss(const std::shared_ptr<EnemyBoss>& boss)
{
    encounterBoss = boss;
    bossGaugeVisible = true;
    bossGaugeIntroTime = 0.0f;
    bossHealthTrail.Reset(0.0f);
}

bool HealthGaugeUI::IsBossIntroPlaying() const
{
    const auto boss = encounterBoss.lock();
    return bossGaugeVisible && boss && !boss->IsDead() && bossGaugeIntroTime < BOSS_GAUGE_INTRO_DURATION;
}

// 更新処理
void HealthGaugeUI::Update(const Player& player, float elapsedTime)
{
    if (bossGaugeVisible && !player.IsDead())
    {
        bossGaugeIntroTime =
            (std::min)(BOSS_GAUGE_INTRO_DURATION, bossGaugeIntroTime + (std::max)(0.0f, elapsedTime));
    }

    playerHealthTrail.Update(static_cast<float>(player.GetHealth()) / (std::max)(1, player.GetMaxHealth()),
                             elapsedTime);

    if (bossGaugeVisible)
    {
        const auto boss = encounterBoss.lock();
        const float ratio =
            boss ? static_cast<float>(boss->GetHealth()) / (std::max)(1, boss->GetMaxHealth()) : 0.0f;

        if (boss && !boss->IsDead() && bossGaugeIntroTime < BOSS_GAUGE_INTRO_DURATION)
        {
            bossHealthTrail.Reset(bossGaugeIntroTime / BOSS_GAUGE_INTRO_DURATION);
        }
        else
        {
            bossHealthTrail.Update(ratio, elapsedTime);
        }
    }
}

void HealthGaugeUI::EnqueueRender()
{

    auto* hud = ImGui::GetForegroundDrawList();
    hud->AddCallback(
        [](const ImDrawList*, const ImDrawCmd* command)
        {

            static_cast<HealthGaugeUI*>(command->UserCallbackData)->Render();
        },
        this);
    hud->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

// 描画処理
void HealthGaugeUI::Render()
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
    auto* states = graphics.GetRenderState();
    dc->OMSetBlendState(states->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(states->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = states->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);
    dc->PSSetConstantBuffers(1, 1, gaugeConstants.GetAddressOf());
    const float scale = (std::min)(graphics.GetScreenWidth() / 1280.0f, graphics.GetScreenHeight() / 720.0f);
    const float height = 300.0f * scale;
    const float margin = 24.0f * scale;
    const auto draw = [&](const Sprite& sprite, float x, float width, const DirectX::XMFLOAT4& mask,
                          const DirectX::XMFLOAT4& color, const HealthGaugeTrail& health)
    {

        const DirectX::XMFLOAT4 data[] = {mask, color, {health.current, health.trailing, 0, 0}};
        dc->UpdateSubresource(gaugeConstants.Get(), 0, nullptr, data, 0, 0);
        sprite.Render(dc, x, margin, 0, width, height, 0, 1, 1, 1, 1);
    };
    draw(*playerGauge, margin, height * 807.0f / 1949.0f,
         {260.0f / 807, 80.0f / 1949, 535.0f / 807, 1270.0f / 1949}, {0.12f, 0.95f, 0.28f, 1},
         playerHealthTrail);

    if (bossGaugeVisible)
    {
        const float width = height * 724.0f / 2171.0f;
        draw(*bossGauge, graphics.GetScreenWidth() - margin - width, width,
             {310.0f / 724, 95.0f / 2171, 635.0f / 724, 1405.0f / 2171}, {0.72f, 0.20f, 1.0f, 1},
             bossHealthTrail);
    }

    ID3D11Buffer* noBuffer = nullptr;
    dc->PSSetConstantBuffers(1, 1, &noBuffer);
}
