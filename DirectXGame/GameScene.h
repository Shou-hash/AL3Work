#pragma once
#include "BaseEnemy.h"
#include "CameraController.h"
#include "DeathParticles.h"
#include "Fade.h"
#include "HitEffect.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
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

	void GenerateBlocks();
	bool isFinished() const { return finished_; }

	void ChangePhase();
	void UpdateFadeIn();
	void UpdatePlay();
	void UpdateDeath();
	void UpdateFadeOut();

private:
	KamataEngine::Model* modelHitEffect_ = nullptr;
	std::list<HitEffect*> hitEffects_;

	Phase phase_ = Phase::kFadeIn;
	bool finished_ = false;
	Fade* fade_ = nullptr;

	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_{};
	KamataEngine::WorldTransform worldTransform_;
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;

	std::unique_ptr<Skydome> skydome = nullptr;
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<CameraController> cameraController_ = nullptr;

	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	MapChipField* mapChipField_;

	// ポリモーフィズム対応：全種類の敵を統一管理するリスト
	std::list<BaseEnemy*> enemies_;

	KamataEngine::Model* modelDeathParticles_ = nullptr;
	std::unique_ptr<DeathParticles> deathParticles_;
};