#include "Audio/AudioResource.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

static void require(bool value) { if (!value) throw std::runtime_error("WAV test failed"); }
static void put32(std::vector<unsigned char>& b, unsigned int v) { for (int i=0;i<4;++i) b.push_back(static_cast<unsigned char>(v >> (i*8))); }
static void chunk(std::vector<unsigned char>& b, const char* tag, const std::vector<unsigned char>& data)
{
    b.insert(b.end(), tag, tag+4); put32(b, static_cast<unsigned int>(data.size()));
    b.insert(b.end(), data.begin(), data.end()); if (data.size() & 1) b.push_back(0);
}
int main()
{
    try {
        int count = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("Data/Audio"))
            if (entry.path().extension() == ".wav") { AudioResource wav(entry.path().string().c_str()); require(wav.GetAudioBytes() > 0); ++count; }
        std::vector<unsigned char> body{'W','A','V','E'};
        chunk(body, "JUNK", {42}); // Odd-length chunks require padding.
        chunk(body, "data", {0,128,255}); // Data is allowed before fmt.
        chunk(body, "fmt ", {1,0,1,0,0x40,0x1f,0,0,0x40,0x1f,0,0,1,0,8,0});
        std::vector<unsigned char> bytes{'R','I','F','F'}; put32(bytes, static_cast<unsigned int>(body.size())); bytes.insert(bytes.end(),body.begin(),body.end());
        auto save = [&]() { std::ofstream f("obj/audio_resource_test.wav",std::ios::binary); f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); };
        save();
        AudioResource pcm("obj/audio_resource_test.wav");
        require(pcm.GetWaveFormat().cbSize == 0 && pcm.GetAudioBytes() == 3);
        require(pcm.GetAudioData()[0] == 0 && pcm.GetAudioData()[1] == 128 && pcm.GetAudioData()[2] == 255);
        bytes.pop_back(); save();
        bool rejected = false; try { AudioResource bad("obj/audio_resource_test.wav"); } catch (const std::runtime_error&) { rejected = true; } require(rejected);
        rejected = false; try { AudioResource missing("obj/no-such-audio-file.wav"); } catch (const std::runtime_error&) { rejected = true; } require(rejected);
        std::cout << "PASS: " << count << " project WAV files; unsigned PCM, chunk padding/order, truncation, missing file\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
