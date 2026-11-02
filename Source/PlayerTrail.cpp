#include "PlayerTrail.h"
#include "SwordTrailRenderer.h"
#include "ModelRenderer.h"
#include <algorithm>
#include <cmath>

void PlayerTrail::RecordMotion(float elapsedTime, const Model& model, const DirectX::XMFLOAT3& position,
                               const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scale)
{
    trailRecordTimer -= elapsedTime;

    if (trailRecordTimer <= 0.0f)
    {
        trailRecordTimer = TRAIL_INTERVAL;
        MotionSnapshot data;
        data.trailModel = std::make_shared<Model>(model);
        data.position = position;
        data.angle = rotation;
        data.scale = scale;
        data.alpha = 0.5f;
        data.drawColor = {1.0f, 0.0f, 0.0f, 1.0f};

        trails.push_back(data);

        if (trails.size() > MAX_TRAILS)
        {
            trails.pop_front();
        }
    }
}

void PlayerTrail::RecordSword(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip, bool reset,
                              float elapsedTime)
{
    trailFadeTimer = TRAIL_FADE_DURATION;

    // 攻撃が開始された最初のフレームですべての座標を現在の位置で初期化する
    if (reset)
    {
        for (SwordSample& sample : swordSamples)
        {
            sample = {root, tip, 0.0f};
        }

        swordSampleCount = 1;
        return;
    }

    for (int i = MAX_SWORD_SAMPLES - 1; i > 0; i--)
    {
        swordSamples[i] = swordSamples[i - 1];
        swordSamples[i].age += elapsedTime;
    }

    swordSamples[0] = {root, tip, 0.0f};
    swordSampleCount = (std::min)(swordSampleCount + 1, MAX_SWORD_SAMPLES);
}

void PlayerTrail::FadeSword(float elapsedTime)
{
    if (trailFadeTimer > 0.0f)
    {
        trailFadeTimer -= elapsedTime;
    }
}

// RenderSword の描画処理
void PlayerTrail::RenderSword(SwordTrailRenderer* swordTrailRenderer) const
{
    const int division = 8;
    const float handleClearance = 0.12f;
    const float glowMargin = 0.12f;

    // 寿命内のサンプルだけを使い、最後は寿命ちょうどの位置へ補間して長さをそろえる
    SwordSample samples[MAX_SWORD_SAMPLES];
    int count = 0;

    for (int i = 0; i < swordSampleCount; i++)
    {
        const SwordSample& sample = swordSamples[i];

        if (sample.age <= SWORD_TRAIL_LIFETIME)
        {
            samples[count++] = sample;
            continue;
        }

        if (i > 0)
        {
            const SwordSample& newer = swordSamples[i - 1];
            const float t = (SWORD_TRAIL_LIFETIME - newer.age) / (sample.age - newer.age);
            DirectX::XMFLOAT3 root, tip;
            DirectX::XMStoreFloat3(&root, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&newer.root),
                                                                DirectX::XMLoadFloat3(&sample.root), t));
            DirectX::XMStoreFloat3(&tip, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&newer.tip),
                                                               DirectX::XMLoadFloat3(&sample.tip), t));
            samples[count++] = {root, tip, SWORD_TRAIL_LIFETIME};
        }

        break;
    }

    if (count < 2)
    {
        return;
    }

    // 刃は回転するので、根元と刃先を別々に直線補間すると弧が角ばる。根元の位置と刃の向きに分けて補間する
    DirectX::XMVECTOR roots[MAX_SWORD_SAMPLES];
    DirectX::XMVECTOR directions[MAX_SWORD_SAMPLES];
    float lengths[MAX_SWORD_SAMPLES];

    for (int i = 0; i < count; i++)
    {
        roots[i] = DirectX::XMLoadFloat3(&samples[i].root);
        const DirectX::XMVECTOR blade =
            DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&samples[i].tip), roots[i]);
        lengths[i] = DirectX::XMVectorGetX(DirectX::XMVector3Length(blade));
        directions[i] = lengths[i] > 0.0001f ? DirectX::XMVectorScale(blade, 1.0f / lengths[i])
                                             : DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }

    SwordTrailRenderer::Params params{};
    params.fade = std::clamp(1.0f - trailFadeTimer / TRAIL_FADE_DURATION, 0.0f, 1.0f);
    params.glowMargin = glowMargin;
    swordTrailRenderer->Begin(params);

    for (int i = 0; i < count - 1; i++)
    {
        const int i0 = (std::max)(0, i - 1);
        const int i1 = i;
        const int i2 = i + 1;
        const int i3 = (std::min)(count - 1, i + 2);
        const int lastStep = (i == count - 2) ? division : division - 1;

        for (int j = 0; j <= lastStep; j++)
        {
            const float t = j / static_cast<float>(division);
            const float age =
                (samples[i1].age + (samples[i2].age - samples[i1].age) * t) / SWORD_TRAIL_LIFETIME;
            const float length = lengths[i1] + (lengths[i2] - lengths[i1]) * t;

            const DirectX::XMVECTOR root =
                DirectX::XMVectorCatmullRom(roots[i0], roots[i1], roots[i2], roots[i3], t);
            DirectX::XMVECTOR direction = DirectX::XMVectorCatmullRom(directions[i0], directions[i1],
                                                                      directions[i2], directions[i3], t);
            direction = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(direction)) > 0.000001f
                            ? DirectX::XMVector3Normalize(direction)
                            : directions[i1];

            DirectX::XMVECTOR bladeStart =
                DirectX::XMVectorAdd(root, DirectX::XMVectorScale(direction, length * handleClearance));
            DirectX::XMVECTOR bladeEnd =
                DirectX::XMVectorAdd(root, DirectX::XMVectorScale(direction, length));

            // 古い部分ほど細くして、尾が自然にすぼまるようにする
            const float taper = 1.0f - 0.3f * age;
            const DirectX::XMVECTOR mid =
                DirectX::XMVectorScale(DirectX::XMVectorAdd(bladeStart, bladeEnd), 0.5f);
            bladeStart = DirectX::XMVectorLerp(mid, bladeStart, taper);
            bladeEnd = DirectX::XMVectorLerp(mid, bladeEnd, taper);
            const DirectX::XMVECTOR glowEnd = DirectX::XMVectorAdd(
                bladeEnd,
                DirectX::XMVectorScale(DirectX::XMVectorSubtract(bladeEnd, bladeStart), glowMargin));

            DirectX::XMFLOAT3 start, end;
            DirectX::XMStoreFloat3(&start, bladeStart);
            DirectX::XMStoreFloat3(&end, glowEnd);
            swordTrailRenderer->AddVertex(start, {0.0f, age});
            swordTrailRenderer->AddVertex(end, {1.0f + glowMargin, age});
        }
    }
}

// RenderMotion の描画処理
void PlayerTrail::RenderMotion(ModelRenderer* modelRenderer) const
{
    for (const auto& trail : trails)
    {
        if (trail.trailModel)
        {
            DirectX::XMMATRIX s = DirectX::XMMatrixScaling(trail.scale.x, trail.scale.y, trail.scale.z);
            DirectX::XMMATRIX r =
                DirectX::XMMatrixRotationRollPitchYaw(trail.angle.x, trail.angle.y, trail.angle.z);
            DirectX::XMMATRIX t =
                DirectX::XMMatrixTranslation(trail.position.x, trail.position.y, trail.position.z);
            DirectX::XMFLOAT4X4 world;
            DirectX::XMStoreFloat4x4(&world, s * r * t);

            trail.trailModel->UpdateTransform(world);
            modelRenderer->Draw(ShaderId::Model, trail.trailModel, trail.drawColor, trail.alpha);
        }
    }
}
