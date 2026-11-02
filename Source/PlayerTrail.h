#pragma once
#include <deque>
#include <memory>
#include "Model.h"

class SwordTrailRenderer;
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

    void RecordSword(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip, bool reset,
                     float elapsedTime = 0.0f);
    void FadeSword(float elapsedTime);

    const DirectX::XMFLOAT3& GetPreviousRoot() const
    {
        return swordSamples[1].root;
    }

    const DirectX::XMFLOAT3& GetPreviousTip() const
    {
        return swordSamples[1].tip;
    }

    // 攻撃終了後の消える演出中も描画し続けるため
    bool IsSwordVisible() const
    {
        return swordSampleCount > 0 && trailFadeTimer > 0.0f;
    }

    // 攻撃終了時に軌跡全体から光の粒を散らすため
    int GetSwordSampleCount() const
    {
        return swordSampleCount;
    }

    const DirectX::XMFLOAT3& GetSwordRoot(int index) const
    {
        return swordSamples[index].root;
    }

    const DirectX::XMFLOAT3& GetSwordTip(int index) const
    {
        return swordSamples[index].tip;
    }

    // RenderSword の描画処理
    void RenderSword(SwordTrailRenderer* swordTrailRenderer) const;

    // RenderMotion の描画処理
    void RenderMotion(ModelRenderer* modelRenderer) const;

  private:

    struct SwordSample
    {
        DirectX::XMFLOAT3 root;
        DirectX::XMFLOAT3 tip;
        float age;
    };

    static constexpr int MAX_SWORD_SAMPLES = 64;
    static constexpr int MAX_TRAILS = 3;
    static constexpr float TRAIL_INTERVAL = 0.05f;
    static constexpr float TRAIL_FADE_DURATION = 0.3f;
    // フレーム数ではなく時間で長さを決め、FPSが変わっても同じ長さにする
    static constexpr float SWORD_TRAIL_LIFETIME = 0.18f;
    SwordSample swordSamples[MAX_SWORD_SAMPLES]{};
    int swordSampleCount = 0;
    std::deque<MotionSnapshot> trails;
    float trailRecordTimer = 0.0f;
    float trailFadeTimer = 0.0f;
};
