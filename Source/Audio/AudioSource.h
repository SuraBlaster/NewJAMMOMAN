#pragma once

#include <memory>
#include <xaudio2.h>
#include "Audio/AudioResource.h"

// オーディオソース
class AudioSource
{
public:
	AudioSource(IXAudio2* xaudio, const std::shared_ptr<AudioResource>& resource);
	~AudioSource();
	AudioSource(const AudioSource&) = delete;
	AudioSource& operator=(const AudioSource&) = delete;

	// 再生
	void Play(bool loop = false);

	// 停止
	void Stop();

	//音量設定
	void SetVolume(float volume);

private:
	IXAudio2SourceVoice* FindFreeVoice();

private:
	IXAudio2* xaudio = nullptr;

	std::shared_ptr<AudioResource>	resource;
	std::vector<IXAudio2SourceVoice*> voices;
	static const int MAX_POLYPHONY = 16;
	float volume = 1.0f;
};
