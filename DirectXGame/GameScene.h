#pragma once
#include "BaseEffect.h" // ★ 基底クラスのインクルード
#include "BaseEnemy.h"
#include "CameraController.h"
#include "Enemy.h"
#include "Fade.h"
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

	void GenerateFieldObjects();
	void GenerateEnemy(uint32_t xIndex, uint32_t yIndex);

	bool isFinished() const { return finished_; }
	bool IsReloadRequested() const { return reloadRequested_; }

	void ChangePhase();
	void UpdateFadeIn();
	void UpdatePlay();
	void UpdateDeath();
	void UpdateFadeOut();

private:
	Phase phase_ = Phase::kFadeIn;
	bool finished_ = false;

	Fade* fade_ = nullptr;

	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_{};
	KamataEngine::WorldTransform worldTransform_;
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// モデルポインタ
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelDeathParticles_ = nullptr;
	KamataEngine::Model* modelHitEffect_ = nullptr;

	std::unique_ptr<Skydome> skydome = nullptr;
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<CameraController> cameraController_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	// ポリモーフィズムで管理する敵とエフェクトのリスト
	std::list<BaseEnemy*> enemies_;
	std::list<BaseEffect*> effects_; // ★ 統合されたエフェクトリスト

	bool reloadRequested_ = false;
};