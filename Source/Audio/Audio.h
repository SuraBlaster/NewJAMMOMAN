#pragma once

#include <xaudio2.h>
#include "Audio/AudioSource.h"
#include <fstream>
#include <sstream>
#include <string>
#include <map>

// オーディオ
class Audio
{
public:
	Audio();
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;
	~Audio();

public:
	// インスタンス取得
	static Audio& Instance() { return *instance; }

	// 音源の登録
	void Register(const std::string& key, const char* filename);

	// リストファイルから一括登録
	void RegisterFromFile(const char* listFilename);

	// 登録した音源の再生
	void Play(const std::string& key, bool loop = false);

	// 登録した音源の停止
	void Stop(const std::string& key);

	// 全停止
	void StopAll();

	// マスターボリューム設定
	void SetMasterVolume(float volume);

	// 個別の音量設定
	void SetVolume(const std::string& key, float volume);

	// オーディオソース読み込み
	std::unique_ptr<AudioSource> LoadAudioSource(const char* filename);

private:
	bool comInitialized = false;
	static Audio*			instance;

	IXAudio2*				xaudio = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;

	std::map<std::string, std::shared_ptr<AudioSource>> soundMap;
};
