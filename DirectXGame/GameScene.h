#pragma once
#include "Kamataengine.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "Skydome.h"
#include "Enemy.h"
#include "CameraController.h"
#include <memory>
#include <vector>

class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();
	KamataEngine::Camera& GetCamera() { return camera_; }

	void GenerateBlocks();

private:
	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_{};
	KamataEngine::WorldTransform worldTransform_;
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 敵（Wizard）の3Dモデルポインタ
	KamataEngine::Model* modelEnemy_ = nullptr;

	// 敵のインスタンスを unique_ptr で安全に管理（Playerと同様の構成）
	std::unique_ptr<Enemy> enemy_ = nullptr;

	std::unique_ptr<Skydome> skydome = nullptr;

	// ポインタではなく、安全な unique_ptr で管理
	std::unique_ptr<Player> player_ = nullptr;

	std::unique_ptr<CameraController> cameraController_ = nullptr;

	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	MapChipField* mapChipField_;
};