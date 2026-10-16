#pragma once
#include <deque>
#include <memory>
#include "Model.h"

class PrimitiveRenderer;
class ModelRenderer;

class PlayerTrail
{
  public:

    struct MotionSnapshot
    {
        std::shared_ptr<Model> trailModel;
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 angle;
        DirectX::XMFLOAT3 scale;
        DirectX::XMFLOAT4 drawColor;
        std::vector<Model::NodePose> nodePoses;
        float alpha;
    };

    void RecordMotion(float elapsedTime, const Model& model, const DirectX::XMFLOAT3& position,
                      const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scale);

    void ClearMotion()
    {
        trails.clear();
    }

    const std::deque<MotionSnapshot>& GetMotionSnapshots() const
    {
        return trails;
    }

    void RecordSword(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip, bool reset);
    void FadeSword(float elapsedTime);

    const DirectX::XMFLOAT3& GetPreviousRoot() const
    {
        return trailPositions[0][1];
    }

    const DirectX::XMFLOAT3& GetPreviousTip() const
    {
        return trailPositions[1][1];
    }

    // RenderSword の描画処理
    void RenderSword(PrimitiveRenderer* primitiveRenderer) const;

    // RenderMotion の描画処理
    void RenderMotion(ModelRenderer* modelRenderer) const;

  private:

    static constexpr int MAX_POLYGON = 32;
    static constexpr int MAX_TRAILS = 3;
    static constexpr float TRAIL_INTERVAL = 0.05f;
    static constexpr float TRAIL_FADE_DURATION = 0.25f;
    DirectX::XMFLOAT3 trailPositions[2][MAX_POLYGON];
    std::deque<MotionSnapshot> trails;
    float trailRecordTimer = 0.0f;
    float trailFadeTimer = 0.0f;
};
