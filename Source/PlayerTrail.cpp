#include "PlayerTrail.h"
#include "PrimitiveRenderer.h"
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

void PlayerTrail::RecordSword(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip, bool reset)
{
    trailFadeTimer = TRAIL_FADE_DURATION;
    // 攻撃が開始された最初のフレームですべての座標を現在の位置で初期化する
    if (reset)
    {
        for (int i = 0; i < MAX_POLYGON; i++)
        {
            trailPositions[0][i] = root;
            trailPositions[1][i] = tip;
        }
    }
    else
    {
        // アクティブ中は頂点座標を1フレーム分ずらして最新を更新する
        for (int i = 0; i < 2; i++)
        {
            for (int j = MAX_POLYGON - 1; j > 0; j--)
            {
                trailPositions[i][j] = trailPositions[i][j - 1];
            }
        }

        trailPositions[0][0] = root;
        trailPositions[1][0] = tip;
    }
}

void PlayerTrail::FadeSword(float elapsedTime)
{
    if (trailFadeTimer > 0.0f)
    {
        trailFadeTimer -= elapsedTime;
    }
}

// RenderSword の描画処理
void PlayerTrail::RenderSword(PrimitiveRenderer* primitiveRenderer) const
{
    const int segmentCount = MAX_POLYGON - 1;
    const int division = 20;
    const float trailHalfDepth = 0.10f;
    const float handleClearance = 0.12f;
    const DirectX::XMFLOAT3 rootColor = {1.00f, 1.00f, 0.92f};
    const DirectX::XMFLOAT3 tipColor = {0.08f, 1.00f, 0.38f};

    auto addTriangle =
        [&](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT2& uvA,
            const DirectX::XMFLOAT3& b, const DirectX::XMFLOAT4& colorB, const DirectX::XMFLOAT2& uvB,
            const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT4& colorC, const DirectX::XMFLOAT2& uvC)
    {
        primitiveRenderer->AddVertex(a, colorA, uvA);
        primitiveRenderer->AddVertex(b, colorB, uvB);
        primitiveRenderer->AddVertex(c, colorC, uvC);
    };

    auto addQuad =
        [&](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT2& uvA,
            const DirectX::XMFLOAT3& b, const DirectX::XMFLOAT4& colorB, const DirectX::XMFLOAT2& uvB,
            const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT4& colorC, const DirectX::XMFLOAT2& uvC,
            const DirectX::XMFLOAT3& d, const DirectX::XMFLOAT4& colorD, const DirectX::XMFLOAT2& uvD)
    {
        addTriangle(a, colorA, uvA, b, colorB, uvB, c, colorC, uvC);
        addTriangle(a, colorA, uvA, c, colorC, uvC, d, colorD, uvD);
    };

    bool hasPreviousSlice = false;

    for (int i = 0; i < segmentCount; i++)
    {
        const int idx0 = (std::max)(0, i - 1);
        const int idx1 = i;
        const int idx2 = (std::min)(MAX_POLYGON - 1, i + 1);
        const int idx3 = (std::min)(MAX_POLYGON - 1, i + 2);

        const DirectX::XMVECTOR root0 = DirectX::XMLoadFloat3(&trailPositions[0][idx0]);
        const DirectX::XMVECTOR root1 = DirectX::XMLoadFloat3(&trailPositions[0][idx1]);
        const DirectX::XMVECTOR root2 = DirectX::XMLoadFloat3(&trailPositions[0][idx2]);
        const DirectX::XMVECTOR root3 = DirectX::XMLoadFloat3(&trailPositions[0][idx3]);
        const DirectX::XMVECTOR tip0 = DirectX::XMLoadFloat3(&trailPositions[1][idx0]);
        const DirectX::XMVECTOR tip1 = DirectX::XMLoadFloat3(&trailPositions[1][idx1]);
        const DirectX::XMVECTOR tip2 = DirectX::XMLoadFloat3(&trailPositions[1][idx2]);
        const DirectX::XMVECTOR tip3 = DirectX::XMLoadFloat3(&trailPositions[1][idx3]);

        const int endJ = (i == segmentCount - 1) ? division : division - 1;

        for (int j = 0; j <= endJ; j++)
        {
            const float t = j / static_cast<float>(division);
            const float age = (i + t) / static_cast<float>(segmentCount);

            DirectX::XMVECTOR root = DirectX::XMVectorCatmullRom(root0, root1, root2, root3, t);
            DirectX::XMVECTOR tip = DirectX::XMVectorCatmullRom(tip0, tip1, tip2, tip3, t);

            const float fadeMul = 1.0f;
            const float fadeShape = 1.0f;
            const float taper = powf(1.0f - age, 1.15f) * fadeShape;
            const DirectX::XMVECTOR mid = DirectX::XMVectorScale(DirectX::XMVectorAdd(root, tip), 0.5f);
            root = DirectX::XMVectorLerp(mid, root, taper);
            tip = DirectX::XMVectorLerp(mid, tip, taper);
            root = DirectX::XMVectorLerp(root, tip, handleClearance);

            const float baseAlpha = powf(1.0f - age, 1.65f);
            const float alpha = baseAlpha * fadeMul * fadeMul * fadeMul;
            const float greenShift = age * 0.35f;
            const DirectX::XMFLOAT4 rootDrawColor = {
                rootColor.x * (1.0f - greenShift) + tipColor.x * greenShift,
                rootColor.y * (1.0f - greenShift) + tipColor.y * greenShift,
                rootColor.z * (1.0f - greenShift) + tipColor.z * greenShift, alpha * 0.15f};
            const DirectX::XMFLOAT4 tipDrawColor = {tipColor.x, tipColor.y, tipColor.z, alpha};
            DirectX::XMFLOAT4 rootSideColor = rootDrawColor;
            DirectX::XMFLOAT4 tipSideColor = tipDrawColor;
            rootSideColor.w *= 0.40f;
            tipSideColor.w *= 0.40f;

            DirectX::XMVECTOR sweepDirection = DirectX::XMVectorAdd(DirectX::XMVectorSubtract(root2, root1),
                                                                    DirectX::XMVectorSubtract(tip2, tip1));

            if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(sweepDirection)) < 0.0001f)
            {
                sweepDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            }

            sweepDirection = DirectX::XMVector3Normalize(sweepDirection);

            DirectX::XMFLOAT3 rootFront, tipFront;
            DirectX::XMStoreFloat3(&rootFront, root);
            DirectX::XMStoreFloat3(&tipFront, tip);

            if (!hasPreviousSlice)
            {
                const DirectX::XMVECTOR leadingPoint =
                    DirectX::XMVectorSubtract(DirectX::XMVectorScale(DirectX::XMVectorAdd(root, tip), 0.5f),
                                              DirectX::XMVectorScale(sweepDirection, 0.45f));
                DirectX::XMFLOAT3 apex;
                DirectX::XMStoreFloat3(&apex, leadingPoint);
                primitiveRenderer->AddVertex(apex, tipDrawColor, {0.5f, -0.15f});
            }

            primitiveRenderer->AddVertex(rootFront, rootDrawColor, {0.0f, age});
            primitiveRenderer->AddVertex(tipFront, tipDrawColor, {1.0f, age});

            hasPreviousSlice = true;
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
