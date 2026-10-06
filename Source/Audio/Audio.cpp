#include <stdexcept>
#include <cmath>
#include <algorithm>
#include "Misc.h"
#include "Audio/Audio.h"

Audio* Audio::instance = nullptr;

// コンストラクタ
Audio::Audio()
{
	// インスタンス設定
	if (instance) throw std::logic_error("Audio already instantiated");
	instance = this;

	HRESULT hr;

	// COMの初期化
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	comInitialized = SUCCEEDED(hr);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return;

	UINT32 createFlags = 0;

	// XAudio初期化
	hr = XAudio2Create(&xaudio, createFlags);
	if (FAILED(hr)) return;

	// マスタリングボイス生成
	hr = xaudio->CreateMasteringVoice(&masteringVoice);
	if (FAILED(hr)) return;

	RegisterFromFile("Data/Audio/sound_list.csv");
}

// デストラクタ
Audio::~Audio()
{
	// 管理している音源を破棄
	soundMap.clear();

	// マスタリングボイス破棄
	if (masteringVoice != nullptr)
	{
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}

	// XAudio終了化
	if (xaudio != nullptr)
	{
		xaudio->Release();
		xaudio = nullptr;
	}

	// COM終了化
	if (comInitialized) CoUninitialize();
	instance = nullptr;
}

// 音源の登録
void Audio::Register(const std::string& key, const char* filename)
{
	// 既に同じキーで登録されていたら何もしない
	if (soundMap.find(key) != soundMap.end())
	{
		return;
	}

	// 既存のLoadAudioSourceを使って読み込む
	if (auto source = LoadAudioSource(filename)) soundMap.emplace(key, std::move(source));
}

void Audio::RegisterFromFile(const char* listFilename)
{
	std::ifstream file(listFilename);
	if (!file.is_open()) return;

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line.find("//") == 0) continue;

		std::stringstream ss(line);
		std::string key, path, volStr;

		// 1. キー
		std::getline(ss, key, ',');
		// 2. パス
		std::getline(ss, path, ',');
		// 3. 音量 (あれば読み込む)
		std::getline(ss, volStr, ',');

		// パスの改行除去（前回と同じ）
		if (!path.empty() && (path.back() == '\r' || path.back() == '\n')) path.pop_back();

		if (!key.empty() && !path.empty())
		{
			Register(key, path.c_str());

			// 音量の指定があった場合、floatに変換してセット
			if (!volStr.empty())
			{
				try {
					float vol = std::stof(volStr);
					SetVolume(key, vol);
				}
				catch (...) {
					// 数値変換に失敗した場合は無視(1.0のまま)
				}
			}
		}
	}
}

// 登録した音源の再生
void Audio::Play(const std::string& key, bool loop)
{
	// キーが存在するか確認
	if (soundMap.find(key) != soundMap.end())
	{
		// AudioSourceのPlayを呼ぶ
		soundMap[key]->Play(loop);
	}
}

// 登録した音源の停止
void Audio::Stop(const std::string& key)
{
	if (soundMap.find(key) != soundMap.end())
	{
		soundMap[key]->Stop();
	}
}

void Audio::StopAll()
{
	for (auto& pair : soundMap)
	{
		pair.second->Stop();
	}	
}

void Audio::SetMasterVolume(float volume)
{
	if (masteringVoice)
	{
		// 全体の出力音量を変更
		if (std::isfinite(volume)) masteringVoice->SetVolume((std::clamp)(volume, 0.0f, XAUDIO2_MAX_VOLUME_LEVEL));
	}
}

void Audio::SetVolume(const std::string& key, float volume)
{
	auto it = soundMap.find(key);
	if (it != soundMap.end())
	{
		it->second->SetVolume(volume);
	}
}

// オーディオソース読み込み
std::unique_ptr<AudioSource> Audio::LoadAudioSource(const char* filename)
{
	if (!xaudio || !masteringVoice) return nullptr;
	try {
	std::shared_ptr<AudioResource> resource = std::make_shared<AudioResource>(filename);
	return std::make_unique<AudioSource>(xaudio, resource);
	} catch (const std::exception& error) {
		OutputDebugStringA(error.what());
		OutputDebugStringA("\n");
		return nullptr;
	}
}
