#pragma once
#include <vector>
#include <Windows.h>
#include <mmreg.h>

class AudioResource
{
  public:

    // コンストラクタ
    explicit AudioResource(const char* filename);

    // デストラクタ
    ~AudioResource() = default;

    UINT8* GetAudioData()
    {
        return data.data();
    }

    UINT32 GetAudioBytes() const
    {
        return static_cast<UINT32>(data.size());
    }

    const WAVEFORMATEX& GetWaveFormat() const
    {
        return format.Format;
    }

  private:

    std::vector<UINT8> data;
    WAVEFORMATEXTENSIBLE format{};
};
