#pragma once
#include "BaseEnemy.h"
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "HitEffect.h"
#include "Kamataengine.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "ShieldEnemy.h"
#include "Skydome.h"
#include <list>
#include <memory>
#include <vector>

enum class Phase {
	kFadeIn,  // フェードイン
	kPlay,    // ゲームプレイ
	kDeath,   // デス演出
	kFadeOut, // フェードアウト
};

class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();
	KamataEngine::Camera& GetCamera() { return camera_; }

	// 関数名を GenerateBlocks から GenerateFieldObjects に変更
	void GenerateFieldObjects();

	// 敵の個別生成関数
	void GenerateEnemy(uint32_t xIndex, uint32_t yIndex);

	// シーンが終了したかを取得
	bool isFinished() const { return finished_; }

	// フェーズ管理用関数
	void ChangePhase();

	void UpdateFadeIn(); // フェードイン処理

	void UpdatePlay(); // ゲームプレイ処理

	void UpdateDeath(); // デス演出処理

	void UpdateFadeOut(); // フェードアウト処理

private:
	// ヒットエフェクトの3Dモデルポインタ
	KamataEngine::Model* modelHitEffect_ = nullptr;

	// 複数のヒットエフェクトを管理するリスト
	std::list<HitEffect*> hitEffects_;

	// フェイズの状態を管理する変数
	Phase phase_ = Phase::kFadeIn;

	bool finished_ = false;

	Fade* fade_ = nullptr;

	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_{};
	KamataEngine::WorldTransform worldTransform_;
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 敵（Wizard）の3Dモデルポインタ
	KamataEngine::Model* modelEnemy_ = nullptr;

	KamataEngine::Model* modelShieldEnemy_ = nullptr;

	std::unique_ptr<Enemy> enemy_ = nullptr;

	std::unique_ptr<ShieldEnemy> Shieldenemy_ = nullptr;

	std::unique_ptr<Skydome> skydome = nullptr;

	// ポインタではなく、安全な unique_ptr で管理
	std::unique_ptr<Player> player_ = nullptr;

	std::unique_ptr<CameraController> cameraController_ = nullptr;

	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	MapChipField* mapChipField_;

	/// <summary>
	/// 全ての敵を格納するリスト
	/// </summary>
	std::list<BaseEnemy*> enemies_;

	// デスパーティクルの3Dモデルポインタ
	KamataEngine::Model* modelDeathParticles_ = nullptr;
	// デスパーティクル（ユニークポインタ）
	std::unique_ptr<DeathParticles> deathParticles_;
};