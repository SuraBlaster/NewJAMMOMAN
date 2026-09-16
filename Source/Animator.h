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
    std::vector<float> bone_weights;

    // ボーン階層からマスクを自動生成する関数
    void BuildLayerMaskFromRoot(const Model* model, const char* root_bone_name, float target_weight = 1.0f);
};

// アニメーションレイヤー（再生状態の保持）
struct AnimationLayer
{
    // 同時にブレンドするクリップは常に最大2つ
    int clip_0 = -1;
    int clip_1 = -1;

    float current_time = 0.0f;
    float duration = 0.0f;       // clip_0の長さを基準とする
    float end_time = -1.0f;
    float blend_rate = 0.0f;     // 0.0=clip_0のみ, 1.0=clip_1のみ

    float layer_global_weight = 0.0f; // このレイヤーの合成強度
    AnimationBlendMode blend_mode = AnimationBlendMode::Override;
    const AnimationLayerMask* mask = nullptr;

    float blend_time = 0.0f;
    float blend_duration = 0.0f;

    std::vector<Model::NodePose> previous_node_poses; // クロスフェードの開始ポーズ
    std::vector<Model::NodePose> computed_poses;      // このレイヤー内部で計算されたポーズ
    std::vector<Model::NodePose> base_poses;          // Additive用の基準ポーズ(0フレーム目)

    bool is_playing = false;
    bool is_loop = false;

    float playback_speed = 1.0f;
};

class Animator
{
public:
    Animator(Model* model, int max_layers = 1);

    void Update(float elapsed_time);

    // 指定レイヤーでアニメーションの再生を開始（クロスフェード対応）
    void Play(int layer_index, int clip_index, bool loop, float blend_duration = 0.1f, float start_time = 0.0f, float end_time = -1.0f);
    void Play(int layer_index, const char* clip_name, bool loop, float blend_duration = 0.1f, float start_time = 0.0f, float end_time = -1.0f);

    // 再生時間を維持したまま、ブレンド対象の2つのクリップとブレンド率を更新する（ストレイフや歩き/走りブレンド用）
    void SetLayerBlend(int layer_index, int clip_0, int clip_1, float blend_rate);

    // レイヤーの合成状態（全体ウェイト、合成モード、マスク）を設定
    void SetLayerState(int layer_index, float weight, AnimationBlendMode mode, const AnimationLayerMask* mask = nullptr);

    int GetClipIndex(const char* clip_name) const;
    bool IsPlaying(int layer_index) const;

    // 指定レイヤーの現在の再生時間（秒）を取得
    float GetCurrentTimes(int layer_index = 0) const;

    // 指定レイヤーのベースアニメーションの長さ（秒）を取得
    float GetDuration(int layer_index = 0) const;

    // 指定レイヤーの進行度（0.0f: 開始 〜 1.0f: 終了）を取得
    float GetProgress(int layer_index = 0) const;

    void SetLayerSpeed(int layer_index, float speed);

private:
    void EvaluateLayerPoses(AnimationLayer& layer, float elapsed_time);
    void ApplyLayerToFinalPoses(std::vector<Model::NodePose>& io_final_poses, const AnimationLayer& layer);

    void BlendNodePoses(std::vector<Model::NodePose>& dst, const std::vector<Model::NodePose>& src0, const std::vector<Model::NodePose>& src1, float blend_factor);
    void BlendSingleNode(Model::NodePose& dst, const Model::NodePose& src0, const Model::NodePose& src1, float blend_factor);

private:
    Model*                          model = nullptr;
    std::vector<AnimationLayer>     layers;

    std::vector<Model::NodePose>    final_node_poses;
    std::vector<Model::NodePose>    temp_node_poses;
};