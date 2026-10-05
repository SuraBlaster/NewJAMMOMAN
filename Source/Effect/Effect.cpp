#include "Effect.h"
#include "EffectManager.h"
#include <Windows.h>
#include <string>

Effect::Effect(const char* filename)
{
    auto manager = EffectManager::Instance().GetEffekseerManager();
    if (manager.Get() == nullptr || !filename) return;
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, nullptr, 0);
    if (length <= 0) return;
    std::wstring path(length, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, path.data(), length);
    effekseerEffect = Effekseer::Effect::Create(manager, reinterpret_cast<const EFK_CHAR*>(path.c_str()));
    if (effekseerEffect.Get() == nullptr) { OutputDebugStringA("Failed to load effect: "); OutputDebugStringA(filename); OutputDebugStringA("\n"); }
}

Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, float scale)
{
    auto manager = EffectManager::Instance().GetEffekseerManager();
    if (manager.Get() == nullptr || effekseerEffect.Get() == nullptr) return -1;
    auto handle = manager->Play(effekseerEffect, position.x, position.y, position.z);
    if (handle >= 0) manager->SetScale(handle, scale, scale, scale);
    return handle;
}

void Effect::Stop(Effekseer::Handle handle)
{
    auto manager = EffectManager::Instance().GetEffekseerManager();
    if (manager.Get() != nullptr && handle >= 0) manager->StopEffect(handle);
}

void Effect::SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position)
{
    auto manager = EffectManager::Instance().GetEffekseerManager();
    if (manager.Get() != nullptr && handle >= 0) manager->SetLocation(handle, position.x, position.y, position.z);
}

void Effect::SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale)
{
    auto manager = EffectManager::Instance().GetEffekseerManager();
    if (manager.Get() != nullptr && handle >= 0) manager->SetScale(handle, scale.x, scale.y, scale.z);
}

bool Effect::IsPlaying(Effekseer::Handle handle)
{
    auto manager = EffectManager::Instance().GetEffekseerManager();
    return manager.Get() != nullptr && handle >= 0 && manager->Exists(handle);
}

