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

enum class SEType {
	kAttack,     // attack.mp3
	kBossDash,   // bossDush.mp3
	kBossAttack, // bossAttack.mp3
	kDash,       // dush.mp3
	kHummer,     // hummer.mp3
	kStage,      // stage.mp3
};

class AudioManager {
public:
	static AudioManager* GetInstance();

	void Initialize();
	void Finalize();

	void PlayBGM(BGMType type, bool loop = true);
	void StopBGM();

	void PlaySE(SEType type);

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

	// 音声データハンドル（各SEごとに個別の変数で宣言）
	uint32_t attackSeHandle_ = 0;
	uint32_t bossDashSeHandle_ = 0;
	uint32_t bossAttackSeHandle_ = 0;
	uint32_t dashSeHandle_ = 0;
	uint32_t hummerSeHandle_ = 0;
	uint32_t stageSeHandle_ = 0;

	BGMType currentBGM_ = BGMType::kNone;
};