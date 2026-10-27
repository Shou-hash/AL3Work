#pragma once
#include "Kamataengine.h"
#include <cstdint>

enum class BGMType {
	kNone,
	kTitle,
	kSelect,
	kGame,
	kEnd,
	kFinalBoss,  // ★ 追加: ラスボス通常BGM
	kFinalBoss2, // ★ 追加: ラスボス覚醒BGM
	kBossEnemy,  // ★ 追加: 中ボスBGM
};

enum class SEType {
	kAttack,          // attack.mp3
	kBossDash,        // bossDush.mp3
	kBossAttack,      // bossAttack.mp3
	kDash,            // dush.mp3
	kHummer,          // hummer.mp3
	kStage,           // stage.mp3
	kBossSpawn,       // ★ 追加: ラスボス出現.mp3
	kBossSkill,       // ★ 追加: ラスボススキル発動.mp3
	kBossSkillActive, // ★ 追加: ラスボススキル発動時.mp3
	kBossDown,        // ★ 追加: 伏せアニメーション時.mp3
	kBossCrystalHit,  // ★ 追加: ラスボスがクリスタルに攻撃されたとき.mp3
	kBossDisappear,   // ★ 追加: ラスボスが消える時.mp3
	kBossHpZero,      // ★ 追加: ラスボスのHPが0になった時.mp3
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
	uint32_t finalBossBgmHandle_ = 0;  // ★ 追加: ラスボス通常BGMハンドル
	uint32_t finalBoss2BgmHandle_ = 0; // ★ 追加: ラスボス覚醒BGMハンドル
	uint32_t bossEnemyBgmHandle_ = 0;  // ★ 追加: 中ボスBGMハンドル

	// 音声再生ハンドル（各BGMごとに個別の変数で宣言）
	uint32_t titleVoiceHandle_ = 0;
	uint32_t selectVoiceHandle_ = 0;
	uint32_t endVoiceHandle_ = 0;
	uint32_t finalBossVoiceHandle_ = 0;  // ★ 追加: ラスボス通常再生ハンドル
	uint32_t finalBoss2VoiceHandle_ = 0; // ★ 追加: ラスボス覚醒再生ハンドル
	uint32_t bossEnemyVoiceHandle_ = 0;  // ★ 追加: 中ボス再生ハンドル

	// 音声データハンドル（各SEごとに個別の変数で宣言）
	uint32_t attackSeHandle_ = 0;
	uint32_t bossDashSeHandle_ = 0;
	uint32_t bossAttackSeHandle_ = 0;
	uint32_t dashSeHandle_ = 0;
	uint32_t hummerSeHandle_ = 0;
	uint32_t stageSeHandle_ = 0;
	uint32_t bossSpawnSeHandle_ = 0;       // ★ 追加: ラスボス出現SEハンドル
	uint32_t bossSkillSeHandle_ = 0;       // ★ 追加: ラスボススキル発動SEハンドル
	uint32_t bossSkillActiveSeHandle_ = 0; // ★ 追加: ラスボススキル発動時SEハンドル
	uint32_t bossDownSeHandle_ = 0;        // ★ 追加: 伏せアニメーション時SEハンドル
	uint32_t bossCrystalHitSeHandle_ = 0;  // ★ 追加: ラスボスがクリスタルに攻撃されたときSEハンドル
	uint32_t bossDisappearSeHandle_ = 0;   // ★ 追加: ラスボスが消える時SEハンドル
	uint32_t bossHpZeroSeHandle_ = 0;      // ★ 追加: ラスボスのHPが0になった時SEハンドル

	BGMType currentBGM_ = BGMType::kNone;
};