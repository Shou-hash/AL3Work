#include "AudioManager.h"

AudioManager* AudioManager::GetInstance() {
	static AudioManager instance;
	return &instance;
}

void AudioManager::Initialize() {
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();

	// 各BGM用に個別の変数へ読み込み
	titleBgmHandle_ = audio->LoadWave("./Resources/Sound/title.mp3");
	selectBgmHandle_ = audio->LoadWave("./Resources/Sound/select.mp3");
	endBgmHandle_ = audio->LoadWave("./Resources/Sound/end.mp3");
}

void AudioManager::Finalize() { StopBGM(); }

void AudioManager::PlayBGM(BGMType type, bool loop) {
	if (currentBGM_ == type) {
		return;
	}

	// セレクトシーンとゲームシーンで同じBGMを継続再生する判定
	if ((currentBGM_ == BGMType::kSelect && type == BGMType::kGame) || (currentBGM_ == BGMType::kGame && type == BGMType::kSelect)) {
		currentBGM_ = type;
		return;
	}

	// 切り替え時に前のBGMを停止
	StopBGM();

	currentBGM_ = type;
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();

	if (type == BGMType::kTitle) {
		// 音声再生
		titleVoiceHandle_ = audio->PlayWave(titleBgmHandle_, loop);
	} else if (type == BGMType::kSelect || type == BGMType::kGame) {
		// 音声再生
		selectVoiceHandle_ = audio->PlayWave(selectBgmHandle_, loop);
	} else if (type == BGMType::kEnd) {
		// 音声再生
		endVoiceHandle_ = audio->PlayWave(endBgmHandle_, loop);
	}
}

void AudioManager::StopBGM() {
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();

	// 音声停止（現在再生中のBGMに応じて停止）
	if (currentBGM_ == BGMType::kTitle) {
		audio->StopWave(titleVoiceHandle_);
	} else if (currentBGM_ == BGMType::kSelect || currentBGM_ == BGMType::kGame) {
		audio->StopWave(selectVoiceHandle_);
	} else if (currentBGM_ == BGMType::kEnd) {
		audio->StopWave(endVoiceHandle_);
	}

	currentBGM_ = BGMType::kNone;
}