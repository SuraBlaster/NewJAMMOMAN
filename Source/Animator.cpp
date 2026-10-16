#include <algorithm>
#include <cmath>
#include "Animator.h"

// コンストラクタ
Animator::Animator(Model* model, int maxLayers) : model(model)
{
    size_t nodeCount = model->GetNodes().size();
    finalNodePoses.resize(nodeCount);
    tempNodePoses.resize(nodeCount);

    // 各レイヤーのバッファを初期化
    layers.resize(maxLayers);

    for (auto& layer : layers)
    {
        layer.previousNodePoses.resize(nodeCount);
        layer.computedPoses.resize(nodeCount);
        layer.basePoses.resize(nodeCount);
    }
}

// 更新処理
void Animator::Update(float elapsedTime)
{
    // 1. 各レイヤー内で時間を進め、クリップをブレンドしてポーズを計算
    for (AnimationLayer& layer : layers)
    {
        if (layer.isPlaying)
        {
            EvaluateLayerPoses(layer, elapsedTime);
        }
    }

    // Layer 0 (Base Layer) が再生されていなければ何もしない
    if (!layers[0].isPlaying)
    {
        return;
    }

    // 2. ベースレイヤーの結果を最終ポーズの初期値とする
    finalNodePoses = layers[0].computedPoses;

    // 3. Layer 1 以降を上に合成していく
    for (size_t i = 1; i < layers.size(); ++i)
    {
        if (layers[i].isPlaying && layers[i].layerGlobalWeight > 0.001f)
        {
            ApplyLayerToFinalPoses(finalNodePoses, layers[i]);
        }
    }

    // モデルに最終ポーズを適用
    model->SetNodePoses(finalNodePoses);
}

void Animator::EvaluateLayerPoses(AnimationLayer& layer, float elapsedTime)
{
    float targetEndTime =
        (layer.endTime >= 0.0f) ? (std::min)(layer.endTime, layer.duration) : layer.duration;

    // 時間進行
    layer.currentTime += elapsedTime * layer.playbackSpeed;

    if (layer.currentTime >= targetEndTime)
    {
        if (layer.isLoop && targetEndTime > 0.0f)
        {
            layer.currentTime = std::fmod(layer.currentTime, targetEndTime);
        }
        else
        {
            layer.currentTime = targetEndTime;
            layer.isPlaying = false;
        }
    }

    // clip_0 の評価
    if (layer.clip0 >= 0)
    {
        model->ComputeAnimation(layer.clip0, layer.currentTime, layer.computedPoses);
    }

    // clip_1 が有効、かつブレンド率が0より大きい場合のみ、clip_1を計算して合成
    if (layer.clip1 >= 0 && layer.blendRate > 0.001f)
    {
        model->ComputeAnimation(layer.clip1, layer.currentTime, tempNodePoses);
        BlendNodePoses(layer.computedPoses, layer.computedPoses, tempNodePoses, layer.blendRate);
    }

    // 前のアニメーション（異なるステート）からのクロスフェード
    if (layer.blendTime < layer.blendDuration)
    {
        layer.blendTime += elapsedTime;

        if (layer.blendTime >= layer.blendDuration)
        {
            layer.blendTime = layer.blendDuration;
        }

        float blendFactor = layer.blendTime / layer.blendDuration;
        BlendNodePoses(layer.computedPoses, layer.previousNodePoses, layer.computedPoses, blendFactor);
    }
}

void Animator::ApplyLayerToFinalPoses(std::vector<Model::NodePose>& ioFinalPoses, const AnimationLayer& layer)
{
    for (size_t n = 0; n < ioFinalPoses.size(); ++n)
    {
        // マスクウェイトの取得
        float maskWeight = layer.mask ? layer.mask->boneWeights[n] : 1.0f;
        float finalWeight = maskWeight * layer.layerGlobalWeight;

        if (finalWeight <= 0.001f)
        {
            continue;
        }

        if (layer.blendMode == AnimationBlendMode::Override)
        {
            // 上書き（Override）合成
            BlendSingleNode(ioFinalPoses[n], ioFinalPoses[n], layer.computedPoses[n], finalWeight);
        }
        else if (layer.blendMode == AnimationBlendMode::Additive)
        {
            // 加算（Additive）合成
            const Model::NodePose& base = layer.basePoses[n];
            const Model::NodePose& curr = layer.computedPoses[n];
            Model::NodePose& target = ioFinalPoses[n];

            DirectX::XMVECTOR baseP = DirectX::XMLoadFloat3(&base.position);
            DirectX::XMVECTOR currP = DirectX::XMLoadFloat3(&curr.position);
            DirectX::XMVECTOR baseR = DirectX::XMLoadFloat4(&base.rotation);
            DirectX::XMVECTOR currR = DirectX::XMLoadFloat4(&curr.rotation);

            // 差分計算 (Position: Current - Base)
            DirectX::XMVECTOR diffP = DirectX::XMVectorSubtract(currP, baseP);
            // 差分計算 (Rotation: Inv(Base) * Current)
            DirectX::XMVECTOR diffR =
                DirectX::XMQuaternionMultiply(DirectX::XMQuaternionInverse(baseR), currR);

            // 対象ポーズへの適用
            DirectX::XMVECTOR targetP = DirectX::XMLoadFloat3(&target.position);
            DirectX::XMVECTOR targetR = DirectX::XMLoadFloat4(&target.rotation);

            // ウェイトを適用して加算
            DirectX::XMVECTOR scaledDiffP = DirectX::XMVectorScale(diffP, finalWeight);
            DirectX::XMVECTOR finalDiffR =
                DirectX::XMQuaternionSlerp(DirectX::XMQuaternionIdentity(), diffR, finalWeight);

            targetP = DirectX::XMVectorAdd(targetP, scaledDiffP);
            targetR = DirectX::XMQuaternionMultiply(targetR, finalDiffR);

            DirectX::XMStoreFloat3(&target.position, targetP);
            DirectX::XMStoreFloat4(&target.rotation, targetR);
        }
    }
}

void Animator::Play(int layerIndex, int clipIndex, bool loop, float blendDuration, float startTime,
                    float endTime)
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return;
    }

    auto& layer = layers[layerIndex];

    // 新しいステートに移行する前に、現在の計算結果をクロスフェード開始ポーズとして保存
    layer.previousNodePoses = layer.computedPoses;

    layer.clip0 = clipIndex;
    layer.clip1 = -1; // 最初はブレンド対象なし
    layer.blendRate = 0.0f;

    layer.isLoop = loop;
    layer.isPlaying = true;
    layer.currentTime = startTime;
    layer.endTime = endTime;
    layer.blendTime = 0.0f;
    layer.blendDuration = blendDuration;

    if (clipIndex >= 0)
    {
        layer.duration = model->GetAnimations().at(clipIndex).secondsLength;

        // Additiveモードの場合は基準となる0フレーム目のポーズをキャッシュする
        if (layer.blendMode == AnimationBlendMode::Additive)
        {
            model->ComputeAnimation(clipIndex, 0.0f, layer.basePoses);
        }
    }
}

void Animator::Play(int layerIndex, const char* clipName, bool loop, float blendDuration, float startTime,
                    float endTime)
{
    Play(layerIndex, GetClipIndex(clipName), loop, blendDuration, startTime, endTime);
}

void Animator::SetLayerBlend(int layerIndex, int clip0, int clip1, float blendRate)
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return;
    }

    auto& layer = layers[layerIndex];
    layer.clip0 = clip0;
    layer.clip1 = clip1;
    layer.blendRate = blendRate;

    // 再生時間はリセットせずにそのまま進行する
}

void Animator::SetLayerState(int layerIndex, float weight, AnimationBlendMode mode,
                             const AnimationLayerMask* mask)
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return;
    }

    layers[layerIndex].layerGlobalWeight = weight;
    layers[layerIndex].blendMode = mode;
    layers[layerIndex].mask = mask;
}

int Animator::GetClipIndex(const char* clipName) const
{
    const std::vector<Model::Animation>& clips = model->GetAnimations();

    for (size_t i = 0; i < clips.size(); ++i)
    {
        if (clips.at(i).name == clipName)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool Animator::IsPlaying(int layerIndex) const
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return false;
    }

    return layers[layerIndex].isPlaying;
}

float Animator::GetCurrentTimes(int layerIndex) const
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return 0.0f;
    }

    return layers[layerIndex].currentTime;
}

float Animator::GetDuration(int layerIndex) const
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return 0.0f;
    }

    return layers[layerIndex].duration;
}

float Animator::GetProgress(int layerIndex) const
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return 0.0f;
    }

    const auto& layer = layers[layerIndex];

    if (layer.duration <= 0.0f)
    {
        return 0.0f;
    }

    // 現在時間 / 全体時間 で 0.0f ~ 1.0f の進捗率を計算
    return layer.currentTime / layer.duration;
}

void Animator::SetLayerSpeed(int layerIndex, float speed)
{
    if (layerIndex < 0 || layerIndex >= layers.size())
    {
        return;
    }

    layers[layerIndex].playbackSpeed = speed;
}

void Animator::BlendNodePoses(std::vector<Model::NodePose>& dstPoses,
                              const std::vector<Model::NodePose>& srcPoses0,
                              const std::vector<Model::NodePose>& srcPoses1, float blendFactor)
{
    for (size_t nodeIndex = 0; nodeIndex < srcPoses0.size(); ++nodeIndex)
    {
        BlendSingleNode(dstPoses[nodeIndex], srcPoses0[nodeIndex], srcPoses1[nodeIndex], blendFactor);
    }
}

void Animator::BlendSingleNode(Model::NodePose& dst, const Model::NodePose& src0, const Model::NodePose& src1,
                               float blendFactor)
{
    DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&src0.position);
    DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&src1.position);
    DirectX::XMVECTOR r0 = DirectX::XMLoadFloat4(&src0.rotation);
    DirectX::XMVECTOR r1 = DirectX::XMLoadFloat4(&src1.rotation);
    DirectX::XMVECTOR s0 = DirectX::XMLoadFloat3(&src0.scale);
    DirectX::XMVECTOR s1 = DirectX::XMLoadFloat3(&src1.scale);

    DirectX::XMStoreFloat3(&dst.position, DirectX::XMVectorLerp(p0, p1, blendFactor));
    DirectX::XMStoreFloat4(&dst.rotation, DirectX::XMQuaternionSlerp(r0, r1, blendFactor));
    DirectX::XMStoreFloat3(&dst.scale, DirectX::XMVectorLerp(s0, s1, blendFactor));
}

void AnimationLayerMask::BuildLayerMaskFromRoot(const Model* model, const char* rootBoneName,
                                                float targetWeight)
{
    if (!model)
    {
        return;
    }

    const auto& nodes = model->GetNodes();
    boneWeights.assign(nodes.size(), 0.0f);

    int rootIndex = -1;

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].name == rootBoneName)
        {
            rootIndex = static_cast<int>(i);
            break;
        }
    }

    if (rootIndex == -1)
    {
        return;
    }

    boneWeights[rootIndex] = targetWeight;

    for (size_t i = rootIndex + 1; i < nodes.size(); ++i)
    {
        int pIdx = nodes[i].parentIndex;

        if (pIdx >= rootIndex && boneWeights[pIdx] > 0.001f)
        {
            boneWeights[i] = targetWeight;
        }
    }
}