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
	finalBossBgmHandle_ = audio->LoadWave("./Resources/Sound/FinalBoss.mp3");   // ★ 追加: ラスボス通常BGM読み込み
	finalBoss2BgmHandle_ = audio->LoadWave("./Resources/Sound/FinalBoss2.mp3"); // ★ 追加: ラスボス覚醒BGM読み込み
	bossEnemyBgmHandle_ = audio->LoadWave("./Resources/Sound/BossEnemy.mp3");   // ★ 追加: 中ボスBGM読み込み

	// 各SE用に個別の変数へ読み込み
	attackSeHandle_ = audio->LoadWave("./Resources/Sound/attack.mp3");
	bossDashSeHandle_ = audio->LoadWave("./Resources/Sound/bossDush.mp3");
	bossAttackSeHandle_ = audio->LoadWave("./Resources/Sound/bossAttack.mp3");
	dashSeHandle_ = audio->LoadWave("./Resources/Sound/dush.mp3");
	hummerSeHandle_ = audio->LoadWave("./Resources/Sound/hummer.mp3");
	stageSeHandle_ = audio->LoadWave("./Resources/Sound/stage.mp3");
	bossSpawnSeHandle_ = audio->LoadWave("./Resources/Sound/FinalBossAppears.mp3");                          // ★ 追加: ラスボス出現SE読み込み
	bossSkillSeHandle_ = audio->LoadWave("./Resources/Sound/FinalBossSkillActivated.mp3");                   // ★ 追加: ラスボススキル発動SE読み込み
	bossSkillActiveSeHandle_ = audio->LoadWave("./Resources/Sound/FinalBossSkillActivated2.mp3");            // ★ 追加: ラスボススキル発動時SE読み込み
	bossDownSeHandle_ = audio->LoadWave("./Resources/Sound/DuringTheProneAnimation.mp3");                    // ★ 追加: 伏せアニメーション時SE読み込み
	bossCrystalHitSeHandle_ = audio->LoadWave("./Resources/Sound/finalBossIsAttacked.mp3"); // ★ 追加: ラスボスがクリスタルに攻撃されたときSE読み込み
	bossDisappearSeHandle_ = audio->LoadWave("./Resources/Sound/FinalBossDisappears.mp3");                    // ★ 追加: ラスボスが消える時SE読み込み
	bossHpZeroSeHandle_ = audio->LoadWave("./Resources/Sound/finalBossHP0.mp3");                // ★ 追加: ラスボスのHPが0になった時SE読み込み
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
	} else if (type == BGMType::kFinalBoss) {
		// ★ 追加: ラスボス通常BGM再生
		finalBossVoiceHandle_ = audio->PlayWave(finalBossBgmHandle_, loop);
	} else if (type == BGMType::kFinalBoss2) {
		// ★ 追加: ラスボス覚醒BGM再生
		finalBoss2VoiceHandle_ = audio->PlayWave(finalBoss2BgmHandle_, loop);
	} else if (type == BGMType::kBossEnemy) {
		// ★ 追加: 中ボスBGM再生
		bossEnemyVoiceHandle_ = audio->PlayWave(bossEnemyBgmHandle_, loop);
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
	} else if (currentBGM_ == BGMType::kFinalBoss) {
		// ★ 追加: ラスボス通常BGM停止
		audio->StopWave(finalBossVoiceHandle_);
	} else if (currentBGM_ == BGMType::kFinalBoss2) {
		// ★ 追加: ラスボス覚醒BGM停止
		audio->StopWave(finalBoss2VoiceHandle_);
	} else if (currentBGM_ == BGMType::kBossEnemy) {
		// ★ 追加: 中ボスBGM停止
		audio->StopWave(bossEnemyVoiceHandle_);
	}

	currentBGM_ = BGMType::kNone;
}

void AudioManager::PlaySE(SEType type) {
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();

	if (type == SEType::kAttack) {
		audio->PlayWave(attackSeHandle_, false);
	} else if (type == SEType::kBossDash) {
		audio->PlayWave(bossDashSeHandle_, false);
	} else if (type == SEType::kBossAttack) {
		audio->PlayWave(bossAttackSeHandle_, false);
	} else if (type == SEType::kDash) {
		audio->PlayWave(dashSeHandle_, false);
	} else if (type == SEType::kHummer) {
		audio->PlayWave(hummerSeHandle_, false);
	} else if (type == SEType::kStage) {
		audio->PlayWave(stageSeHandle_, false);
	} else if (type == SEType::kBossSpawn) {
		// ★ 追加: ラスボス出現SE再生
		audio->PlayWave(bossSpawnSeHandle_, false);
	} else if (type == SEType::kBossSkill) {
		// ★ 追加: ラスボススキル発動SE再生
		audio->PlayWave(bossSkillSeHandle_, false);
	} else if (type == SEType::kBossSkillActive) {
		// ★ 追加: ラスボススキル発動時SE再生
		audio->PlayWave(bossSkillActiveSeHandle_, false);
	} else if (type == SEType::kBossDown) {
		// ★ 追加: 伏せアニメーション時SE再生
		audio->PlayWave(bossDownSeHandle_, false);
	} else if (type == SEType::kBossCrystalHit) {
		// ★ 追加: ラスボスがクリスタルに攻撃されたときSE再生
		audio->PlayWave(bossCrystalHitSeHandle_, false);
	} else if (type == SEType::kBossDisappear) {
		// ★ 追加: ラスボスが消える時SE再生
		audio->PlayWave(bossDisappearSeHandle_, false);
	} else if (type == SEType::kBossHpZero) {
		// ★ 追加: ラスボスのHPが0になった時SE再生
		audio->PlayWave(bossHpZeroSeHandle_, false);
	}
}