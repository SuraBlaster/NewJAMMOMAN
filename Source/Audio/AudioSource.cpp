#include "Audio/AudioSource.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

AudioSource::AudioSource(IXAudio2* xaudio, const std::shared_ptr<AudioResource>& resource)
    : xaudio(xaudio), resource(resource)
{
    if (!xaudio || !resource) throw std::invalid_argument("Invalid audio source");
    voices.reserve(MAX_POLYPHONY);
    if (!FindFreeVoice()) throw std::runtime_error("XAudio2 source voice creation failed");
}

AudioSource::~AudioSource()
{
    for (auto* voice : voices) voice->DestroyVoice();
}

void AudioSource::Play(bool loop)
{
    if (loop) Stop();
    auto* voice = FindFreeVoice();
    if (!voice) return;
    voice->Stop();
    voice->FlushSourceBuffers();
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = resource->GetAudioBytes();
    buffer.pAudioData = resource->GetAudioData();
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    voice->SetVolume(volume);
    if (SUCCEEDED(voice->SubmitSourceBuffer(&buffer))) voice->Start();
}

void AudioSource::Stop()
{
    for (auto* voice : voices) { voice->Stop(); voice->FlushSourceBuffers(); }
}

void AudioSource::SetVolume(float value)
{
    if (!std::isfinite(value)) return;
    volume = (std::clamp)(value, 0.0f, XAUDIO2_MAX_VOLUME_LEVEL);
    for (auto* voice : voices) voice->SetVolume(volume);
}

IXAudio2SourceVoice* AudioSource::FindFreeVoice()
{
    for (auto* voice : voices)
    {
        XAUDIO2_VOICE_STATE state{};
        voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (!state.BuffersQueued) return voice;
    }
    if (voices.size() >= MAX_POLYPHONY) return nullptr;
    IXAudio2SourceVoice* voice = nullptr;
    if (FAILED(xaudio->CreateSourceVoice(&voice, &resource->GetWaveFormat()))) return nullptr;
    voice->SetVolume(volume);
    voices.push_back(voice);
    return voice;
}
