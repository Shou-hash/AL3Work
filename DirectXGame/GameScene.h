#pragma once
#include "CameraController.h"
#include "Enemy.h"
#include "Kamataengine.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "Skydome.h"
#include <list> // std::list を使用するため追加
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

	// 単体用の enemy_ は複数管理を行うため使用しませんが、型互換のために残します
	std::unique_ptr<Enemy> enemy_ = nullptr;

	std::unique_ptr<Skydome> skydome = nullptr;

	// ポインタではなく、安全な unique_ptr で管理
	std::unique_ptr<Player> player_ = nullptr;

	std::unique_ptr<CameraController> cameraController_ = nullptr;

	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	MapChipField* mapChipField_;

	std::list<Enemy*> enemies_; // 敵のリスト（複数の敵を管理）
};