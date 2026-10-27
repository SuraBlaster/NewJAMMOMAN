#include "ModelManager.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace DirectX;

static void Check(bool ok)
{
    if (!ok)
    {
        throw std::runtime_error("Animation sampling mismatch");
    }
}

static void Reference(const Model& m, int clip, float t, std::vector<Model::NodePose>& poses)
{
    poses.resize(m.GetNodes().size());
    const auto& animation = m.GetAnimations().at(clip);

    for (size_t n = 0; n < poses.size(); n++)
    {
        const auto& track = animation.nodeAnims.at(n);
        auto& pose = poses[n];
        const auto vectorTrack = [t](const auto& keys, XMFLOAT3& out)
        {
            if (keys.size() == 1)
            {
                out = keys[0].value;
                return;
            }

            for (size_t i = 0; i + 1 < keys.size(); i++)
            {
                if (t >= keys[i].seconds && t <= keys[i + 1].seconds)
                {
                    float r = (t - keys[i].seconds) / (keys[i + 1].seconds - keys[i].seconds);
                    XMStoreFloat3(&out, XMVectorLerp(XMLoadFloat3(&keys[i].value),
                                                     XMLoadFloat3(&keys[i + 1].value), r));
                }
            }
        };
        vectorTrack(track.positionKeyframes, pose.position);
        const auto& keys = track.rotationKeyframes;

        if (keys.size() == 1)
        {
            pose.rotation = keys[0].value;
        }

        for (size_t i = 0; i + 1 < keys.size(); i++)
        {
            if (t >= keys[i].seconds && t <= keys[i + 1].seconds)
            {
                float r = (t - keys[i].seconds) / (keys[i + 1].seconds - keys[i].seconds);
                XMStoreFloat4(&pose.rotation, XMQuaternionSlerp(XMLoadFloat4(&keys[i].value),
                                                                XMLoadFloat4(&keys[i + 1].value), r));
            }
        }

        vectorTrack(track.scaleKeyframes, pose.scale);
    }
}

int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                      D3D11_SDK_VERSION, &device, nullptr, nullptr)));

    try
    {
        for (const char* file : {"Data/Model/Enemy/Swarm08.gltf", "Data/Model/Enemy/Wizard.gltf",
                                 "Data/Model/Enemy/FlyEnemy.gltf", "Data/Model/Enemy/Boss.gltf",
                                 "Data/Model/Jammo/Jammo_Player.gltf"})
        {
            auto model = ModelManager::Instance().CreateInstance(device.Get(), file);
            std::vector<Model::NodePose> oldPose, newPose;

            for (size_t clip = 0; clip < model->GetAnimations().size(); clip++)
            {
                for (int sample = 0; sample <= 8; sample++)
                {
                    const float time = model->GetAnimations()[clip].secondsLength * sample / 8;
                    Reference(*model, int(clip), time, oldPose);
                    model->ComputeAnimation(int(clip), time, newPose);
                    Check(oldPose.size() == newPose.size());

                    for (size_t n = 0; n < oldPose.size(); n++)
                    {
                        const float* a = reinterpret_cast<const float*>(&oldPose[n]);
                        const float* b = reinterpret_cast<const float*>(&newPose[n]);

                        for (int k = 0; k < 10; k++)
                        {
                            Check(std::abs(a[k] - b[k]) < .0005f);
                        }
                    }
                }
            }

            const auto benchmark = [&](bool old)
            {
                const auto begin = std::chrono::steady_clock::now();

                for (int i = 0; i < 500; i++)
                {
                    const float t = model->GetAnimations()[0].secondsLength * (i % 100) / 100;

                    if (old)
                    {
                        Reference(*model, 0, t, oldPose);
                    }
                    else
                    {
                        model->ComputeAnimation(0, t, newPose);
                    }
                }

                return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
                    .count();
            };
            std::cout << file << " | 500 samples: linear=" << benchmark(true)
                      << "ms binary=" << benchmark(false) << "ms\n";
        }

        std::cout << "PASS: all enemy clips match reference at 9 times per clip\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    ModelManager::Instance().Clear();
    CoUninitialize();
}
