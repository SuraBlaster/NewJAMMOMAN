#pragma once
#include <vector>
#include <Windows.h>
#include <mmreg.h>

// Owns uncompressed RIFF/WAVE data, including extensible format metadata.
class AudioResource
{
public:
    explicit AudioResource(const char* filename);
    ~AudioResource() = default;
    UINT8* GetAudioData() { return data.data(); }
    UINT32 GetAudioBytes() const { return static_cast<UINT32>(data.size()); }
    const WAVEFORMATEX& GetWaveFormat() const { return format.Format; }
private:
    std::vector<UINT8> data;
    WAVEFORMATEXTENSIBLE format{};
};
