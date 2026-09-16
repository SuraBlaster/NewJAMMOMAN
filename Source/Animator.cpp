#include <algorithm>
#include <cmath>
#include "Animator.h"

Animator::Animator(Model* model, int max_layers)
    : model(model)
{
    size_t node_count = model->GetNodes().size();
    final_node_poses.resize(node_count);
    temp_node_poses.resize(node_count);

    // 各レイヤーのバッファを初期化
    layers.resize(max_layers);
    for (auto& layer : layers)
    {
        layer.previous_node_poses.resize(node_count);
        layer.computed_poses.resize(node_count);
        layer.base_poses.resize(node_count);
    }
}

void Animator::Update(float elapsed_time)
{
    // 1. 各レイヤー内で時間を進め、クリップをブレンドしてポーズを計算
    for (AnimationLayer& layer : layers)
    {
        if (layer.is_playing)
        {
            EvaluateLayerPoses(layer, elapsed_time);
        }
    }

    // Layer 0 (Base Layer) が再生されていなければ何もしない
    if (!layers[0].is_playing) return;

    // 2. ベースレイヤーの結果を最終ポーズの初期値とする
    final_node_poses = layers[0].computed_poses;

    // 3. Layer 1 以降を上に合成していく
    for (size_t i = 1; i < layers.size(); ++i)
    {
        if (layers[i].is_playing && layers[i].layer_global_weight > 0.001f)
        {
            ApplyLayerToFinalPoses(final_node_poses, layers[i]);
        }
    }

    // モデルに最終ポーズを適用
    model->SetNodePoses(final_node_poses);
}

void Animator::EvaluateLayerPoses(AnimationLayer& layer, float elapsed_time)
{
    float target_end_time = (layer.end_time >= 0.0f) ? (std::min)(layer.end_time, layer.duration) : layer.duration;

    // 時間進行
    layer.current_time += elapsed_time * layer.playback_speed;
    if (layer.current_time >= target_end_time)
    {
        if (layer.is_loop && target_end_time > 0.0f)
        {
            layer.current_time = std::fmod(layer.current_time, target_end_time);
        }
        else
        {
            layer.current_time = target_end_time;
            layer.is_playing = false;
        }
    }

    // clip_0 の評価
    if (layer.clip_0 >= 0)
    {
        model->ComputeAnimation(layer.clip_0, layer.current_time, layer.computed_poses); //[cite: 1]
    }

    // clip_1 が有効、かつブレンド率が0より大きい場合のみ、clip_1を計算して合成
    if (layer.clip_1 >= 0 && layer.blend_rate > 0.001f)
    {
        model->ComputeAnimation(layer.clip_1, layer.current_time, temp_node_poses);
        BlendNodePoses(layer.computed_poses, layer.computed_poses, temp_node_poses, layer.blend_rate);
    }

    // 前のアニメーション（異なるステート）からのクロスフェード
    if (layer.blend_time < layer.blend_duration)
    {
        layer.blend_time += elapsed_time;
        if (layer.blend_time >= layer.blend_duration)
        {
            layer.blend_time = layer.blend_duration;
        }
        float blend_factor = layer.blend_time / layer.blend_duration;
        BlendNodePoses(layer.computed_poses, layer.previous_node_poses, layer.computed_poses, blend_factor);
    }
}

void Animator::ApplyLayerToFinalPoses(std::vector<Model::NodePose>& io_final_poses, const AnimationLayer& layer)
{
    for (size_t n = 0; n < io_final_poses.size(); ++n)
    {
        // マスクウェイトの取得
        float mask_weight = layer.mask ? layer.mask->bone_weights[n] : 1.0f;
        float final_weight = mask_weight * layer.layer_global_weight;

        if (final_weight <= 0.001f) continue;

        if (layer.blend_mode == AnimationBlendMode::Override)
        {
            // 上書き（Override）合成
            BlendSingleNode(io_final_poses[n], io_final_poses[n], layer.computed_poses[n], final_weight);
        }
        else if (layer.blend_mode == AnimationBlendMode::Additive)
        {
            // 加算（Additive）合成
            const Model::NodePose& base = layer.base_poses[n];
            const Model::NodePose& curr = layer.computed_poses[n];
            Model::NodePose& target = io_final_poses[n];

            DirectX::XMVECTOR baseP = DirectX::XMLoadFloat3(&base.position);
            DirectX::XMVECTOR currP = DirectX::XMLoadFloat3(&curr.position);
            DirectX::XMVECTOR baseR = DirectX::XMLoadFloat4(&base.rotation);
            DirectX::XMVECTOR currR = DirectX::XMLoadFloat4(&curr.rotation);

            // 差分計算 (Position: Current - Base)
            DirectX::XMVECTOR diffP = DirectX::XMVectorSubtract(currP, baseP);
            // 差分計算 (Rotation: Inv(Base) * Current)
            DirectX::XMVECTOR diffR = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionInverse(baseR), currR);

            // 対象ポーズへの適用
            DirectX::XMVECTOR targetP = DirectX::XMLoadFloat3(&target.position);
            DirectX::XMVECTOR targetR = DirectX::XMLoadFloat4(&target.rotation);

            // ウェイトを適用して加算
            DirectX::XMVECTOR scaledDiffP = DirectX::XMVectorScale(diffP, final_weight);
            DirectX::XMVECTOR finalDiffR = DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), diffR, final_weight);

            targetP = DirectX::XMVectorAdd(targetP, scaledDiffP);
            targetR = DirectX::XMQuaternionMultiply(targetR, finalDiffR);

            DirectX::XMStoreFloat3(&target.position, targetP);
            DirectX::XMStoreFloat4(&target.rotation, targetR);
        }
    }
}

void Animator::Play(int layer_index, int clip_index, bool loop, float blend_duration, float start_time, float end_time)
{
    if (layer_index < 0 || layer_index >= layers.size()) return;

    auto& layer = layers[layer_index];

    // 新しいステートに移行する前に、現在の計算結果をクロスフェード開始ポーズとして保存
    layer.previous_node_poses = layer.computed_poses;

    layer.clip_0 = clip_index;
    layer.clip_1 = -1;       // 最初はブレンド対象なし
    layer.blend_rate = 0.0f;

    layer.is_loop = loop;
    layer.is_playing = true;
    layer.current_time = start_time;
    layer.end_time = end_time;
    layer.blend_time = 0.0f;
    layer.blend_duration = blend_duration;

    if (clip_index >= 0)
    {
        layer.duration = model->GetAnimations().at(clip_index).secondsLength;

        // Additiveモードの場合は基準となる0フレーム目のポーズをキャッシュする
        if (layer.blend_mode == AnimationBlendMode::Additive)
        {
            model->ComputeAnimation(clip_index, 0.0f, layer.base_poses);
        }
    }
}

void Animator::Play(int layer_index, const char* clip_name, bool loop, float blend_duration, float start_time, float end_time)
{
    Play(layer_index, GetClipIndex(clip_name), loop, blend_duration, start_time,end_time);
}

void Animator::SetLayerBlend(int layer_index, int clip_0, int clip_1, float blend_rate)
{
    if (layer_index < 0 || layer_index >= layers.size()) return;

    auto& layer = layers[layer_index];
    layer.clip_0 = clip_0;
    layer.clip_1 = clip_1;
    layer.blend_rate = blend_rate;

    // 再生時間はリセットせずにそのまま進行する
}

void Animator::SetLayerState(int layer_index, float weight, AnimationBlendMode mode, const AnimationLayerMask* mask)
{
    if (layer_index < 0 || layer_index >= layers.size()) return;

    layers[layer_index].layer_global_weight = weight;
    layers[layer_index].blend_mode = mode;
    layers[layer_index].mask = mask;
}

int Animator::GetClipIndex(const char* clip_name) const
{
    const std::vector<Model::Animation>& clips = model->GetAnimations();
    for (size_t i = 0; i < clips.size(); ++i)
    {
        if (clips.at(i).name == clip_name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Animator::IsPlaying(int layer_index) const
{
    if (layer_index < 0 || layer_index >= layers.size()) return false;
    return layers[layer_index].is_playing;
}

float Animator::GetCurrentTimes(int layer_index) const
{
    if (layer_index < 0 || layer_index >= layers.size()) return 0.0f;
    return layers[layer_index].current_time;
}

float Animator::GetDuration(int layer_index) const
{
    if (layer_index < 0 || layer_index >= layers.size()) return 0.0f;
    return layers[layer_index].duration;
}

float Animator::GetProgress(int layer_index) const
{
    if (layer_index < 0 || layer_index >= layers.size()) return 0.0f;

    const auto& layer = layers[layer_index];
    if (layer.duration <= 0.0f) return 0.0f;

    // 現在時間 / 全体時間 で 0.0f ~ 1.0f の進捗率を計算
    return layer.current_time / layer.duration;
}

void Animator::SetLayerSpeed(int layer_index, float speed)
{
    if (layer_index < 0 || layer_index >= layers.size()) return;
    layers[layer_index].playback_speed = speed;
}

void Animator::BlendNodePoses(
    std::vector<Model::NodePose>& dst_poses,
    const std::vector<Model::NodePose>& src_poses0,
    const std::vector<Model::NodePose>& src_poses1,
    float blend_factor)
{
    for (size_t node_index = 0; node_index < src_poses0.size(); ++node_index)
    {
        BlendSingleNode(dst_poses[node_index], src_poses0[node_index], src_poses1[node_index], blend_factor);
    }
}

void Animator::BlendSingleNode(
    Model::NodePose& dst,
    const Model::NodePose& src0,
    const Model::NodePose& src1,
    float blend_factor)
{
    DirectX::XMVECTOR P0 = DirectX::XMLoadFloat3(&src0.position);
    DirectX::XMVECTOR P1 = DirectX::XMLoadFloat3(&src1.position);
    DirectX::XMVECTOR R0 = DirectX::XMLoadFloat4(&src0.rotation);
    DirectX::XMVECTOR R1 = DirectX::XMLoadFloat4(&src1.rotation);
    DirectX::XMVECTOR S0 = DirectX::XMLoadFloat3(&src0.scale);
    DirectX::XMVECTOR S1 = DirectX::XMLoadFloat3(&src1.scale);

    DirectX::XMStoreFloat3(&dst.position, DirectX::XMVectorLerp(P0, P1, blend_factor));
    DirectX::XMStoreFloat4(&dst.rotation, DirectX::XMQuaternionSlerp(R0, R1, blend_factor));
    DirectX::XMStoreFloat3(&dst.scale, DirectX::XMVectorLerp(S0, S1, blend_factor));
}

void AnimationLayerMask::BuildLayerMaskFromRoot(const Model* model, const char* root_bone_name, float target_weight)
{
    if (!model) return;
    const auto& nodes = model->GetNodes();
    bone_weights.assign(nodes.size(), 0.0f);

    int root_index = -1;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].name == root_bone_name)
        {
            root_index = static_cast<int>(i);
            break;
        }
    }

    if (root_index == -1) return;

    bone_weights[root_index] = target_weight;

    for (size_t i = root_index + 1; i < nodes.size(); ++i)
    {
        int p_idx = nodes[i].parentIndex;
        if (p_idx >= root_index && bone_weights[p_idx] > 0.001f)
        {
            bone_weights[i] = target_weight;
        }
    }
}