#pragma once
#include "Kamataengine.h"
#include <cstdint>

enum class BGMType {
	kNone,
	kTitle,
	kSelect,
	kGame,
	kEnd,
};

class AudioManager {
public:
	static AudioManager* GetInstance();

	void Initialize();
	void Finalize();

	void PlayBGM(BGMType type, bool loop = true);
	void StopBGM();

private:
	AudioManager() = default;
	~AudioManager() = default;
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	// 音声データハンドル（各BGMごとに個別の変数で宣言）
	uint32_t titleBgmHandle_ = 0;
	uint32_t selectBgmHandle_ = 0;
	uint32_t endBgmHandle_ = 0;

	// 音声再生ハンドル（各BGMごとに個別の変数で宣言）
	uint32_t titleVoiceHandle_ = 0;
	uint32_t selectVoiceHandle_ = 0;
	uint32_t endVoiceHandle_ = 0;

	BGMType currentBGM_ = BGMType::kNone;
};