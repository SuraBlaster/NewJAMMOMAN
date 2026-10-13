#pragma once

#include "Model.h"
#include <vector>
#include <array>
#include <string>
#include <DirectXMath.h>

// アニメーションの合成モード
enum class AnimationBlendMode
{
    Override, // 上書き（通常の合成）
    Additive  // 加算（差分を足し合わせる）
};

// レイヤーマスク（ボーンごとの適用ウェイト）
// ※実体はゲーム側で保持し、Animatorにはポインタを渡す想定
struct AnimationLayerMask
{
    std::vector<float> boneWeights;

    // ボーン階層からマスクを自動生成する関数
    void BuildLayerMaskFromRoot(const Model* model, const char* rootBoneName, float targetWeight = 1.0f);
};

// アニメーションレイヤー（再生状態の保持）
struct AnimationLayer
{
    // 同時にブレンドするクリップは常に最大2つ
    int clip0 = -1;
    int clip1 = -1;

    float currentTime = 0.0f;
    float duration = 0.0f;       // clip_0の長さを基準とする
    float endTime = -1.0f;
    float blendRate = 0.0f;     // 0.0=clip_0のみ, 1.0=clip_1のみ

    float layerGlobalWeight = 0.0f; // このレイヤーの合成強度
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    const AnimationLayerMask* mask = nullptr;

    float blendTime = 0.0f;
    float blendDuration = 0.0f;

    std::vector<Model::NodePose> previousNodePoses; // クロスフェードの開始ポーズ
    std::vector<Model::NodePose> computedPoses;      // このレイヤー内部で計算されたポーズ
    std::vector<Model::NodePose> basePoses;          // Additive用の基準ポーズ(0フレーム目)

    bool isPlaying = false;
    bool isLoop = false;

    float playbackSpeed = 1.0f;
};

class Animator
{
public:
    Animator(Model* model, int maxLayers = 1);

    void Update(float elapsedTime);

    // 指定レイヤーでアニメーションの再生を開始（クロスフェード対応）
    void Play(int layerIndex, int clipIndex, bool loop, float blendDuration = 0.1f, float startTime = 0.0f, float endTime = -1.0f);
    void Play(int layerIndex, const char* clipName, bool loop, float blendDuration = 0.1f, float startTime = 0.0f, float endTime = -1.0f);

    // 再生時間を維持したまま、ブレンド対象の2つのクリップとブレンド率を更新する（ストレイフや歩き/走りブレンド用）
    void SetLayerBlend(int layerIndex, int clip0, int clip1, float blendRate);

    // レイヤーの合成状態（全体ウェイト、合成モード、マスク）を設定
    void SetLayerState(int layerIndex, float weight, AnimationBlendMode mode, const AnimationLayerMask* mask = nullptr);

    int GetClipIndex(const char* clipName) const;
    bool IsPlaying(int layerIndex) const;

    // 指定レイヤーの現在の再生時間（秒）を取得
    float GetCurrentTimes(int layerIndex = 0) const;

    // 指定レイヤーのベースアニメーションの長さ（秒）を取得
    float GetDuration(int layerIndex = 0) const;

    // 指定レイヤーの進行度（0.0f: 開始 〜 1.0f: 終了）を取得
    float GetProgress(int layerIndex = 0) const;

    void SetLayerSpeed(int layerIndex, float speed);

private:
    void EvaluateLayerPoses(AnimationLayer& layer, float elapsedTime);
    void ApplyLayerToFinalPoses(std::vector<Model::NodePose>& ioFinalPoses, const AnimationLayer& layer);

    void BlendNodePoses(std::vector<Model::NodePose>& dst, const std::vector<Model::NodePose>& src0, const std::vector<Model::NodePose>& src1, float blendFactor);
    void BlendSingleNode(Model::NodePose& dst, const Model::NodePose& src0, const Model::NodePose& src1, float blendFactor);

private:
    Model*                          model = nullptr;
    std::vector<AnimationLayer>     layers;

    std::vector<Model::NodePose>    finalNodePoses;
    std::vector<Model::NodePose>    tempNodePoses;
};