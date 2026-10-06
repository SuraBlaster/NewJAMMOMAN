#include "Audio/AudioResource.h"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <xaudio2.h>

AudioResource::AudioResource(const char* filename)
{
    if (!filename) throw std::runtime_error("Missing WAV filename");
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    auto fail = [filename]() { throw std::runtime_error(std::string("Invalid or unsupported WAV: ") + filename); };
    if (!file || file.tellg() < 12) fail();
    const auto fileSize = static_cast<uint64_t>(file.tellg());
    file.seekg(0);
    auto read = [&](void* dest, size_t size) {
        if (!file.read(static_cast<char*>(dest), static_cast<std::streamsize>(size))) fail();
    };
    char riff[4], wave[4];
    UINT32 riffSize;
    read(riff, 4); read(&riffSize, 4); read(wave, 4);
    const uint64_t end = uint64_t(riffSize) + 8;
    if (memcmp(riff, "RIFF", 4) || memcmp(wave, "WAVE", 4) || end > fileSize || end < 12) fail();
    bool hasFormat = false, hasData = false;
    for (uint64_t offset = 12; offset < end;)
    {
        if (end - offset < 8) fail();
        char tag[4]; UINT32 size;
        read(tag, 4); read(&size, 4);
        offset += 8;
        const uint64_t paddedSize = uint64_t(size) + (size & 1);
        if (paddedSize > end - offset) fail();
        if (!memcmp(tag, "fmt ", 4))
        {
            if (hasFormat || size < 16 || size == 17) fail();
            read(&format, (size < sizeof(format)) ? size : sizeof(format));
            if (size == 16) format.Format.cbSize = 0;
            const auto& f = format.Format;
            if (f.cbSize > size - 16 || (size >= 18 && f.cbSize > size - 18)) fail();
            if (f.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                if (size < sizeof(format) || f.cbSize != 22) fail();
                const GUID pcm = {1, 0, 0x0010, {0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
                GUID floating = pcm; floating.Data1 = WAVE_FORMAT_IEEE_FLOAT;
                if (format.SubFormat != pcm && format.SubFormat != floating) fail();
                if (format.SubFormat == floating && f.wBitsPerSample != 32) fail();
                if (!format.Samples.wValidBitsPerSample || format.Samples.wValidBitsPerSample > f.wBitsPerSample) fail();
            }
            else if ((f.wFormatTag != WAVE_FORMAT_PCM && f.wFormatTag != WAVE_FORMAT_IEEE_FLOAT) || f.cbSize != 0) fail();
            if (!f.nChannels || f.nChannels > XAUDIO2_MAX_AUDIO_CHANNELS ||
                f.nSamplesPerSec < XAUDIO2_MIN_SAMPLE_RATE || f.nSamplesPerSec > XAUDIO2_MAX_SAMPLE_RATE ||
                (f.wBitsPerSample != 8 && f.wBitsPerSample != 16 && f.wBitsPerSample != 24 && f.wBitsPerSample != 32) ||
                (f.wFormatTag == WAVE_FORMAT_IEEE_FLOAT && f.wBitsPerSample != 32) ||
                f.nBlockAlign != f.nChannels * (f.wBitsPerSample / 8) ||
                f.nAvgBytesPerSec != uint64_t(f.nSamplesPerSec) * f.nBlockAlign) fail();
            hasFormat = true;
        }
        else if (!memcmp(tag, "data", 4))
        {
            if (hasData || !size || size > XAUDIO2_MAX_BUFFER_BYTES) fail();
            data.resize(size);
            read(data.data(), size);
            hasData = true;
        }
        offset += paddedSize;
        file.seekg(static_cast<std::streamoff>(offset));
    }
    if (!hasFormat || !hasData || data.size() % format.Format.nBlockAlign) fail();
    // 8-bit PCM is unsigned in both WAV and XAudio2; do not shift its samples.
}
