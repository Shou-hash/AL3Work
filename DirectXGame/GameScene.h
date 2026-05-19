#pragma once
#include "Kamataengine.h"
#include "Matrix4x4.h"
#include <vector>
#include "Skydome.h"
#include <memory>
#include "MapChipField.h"
#include "Player.h"


class GameScene {
public:
	~GameScene();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
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

	std::unique_ptr<Skydome> skydome = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;

	MapChipField* mapChipField_;
};