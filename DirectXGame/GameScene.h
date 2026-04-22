#pragma once
#include "Kamataengine.h"
#include "Player.h" // Playerクラスを使用するためにインクルード

class GameScene {
public:
	~GameScene();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

private:
	// カメラ
	KamataEngine::Camera camera_;

	// プレイヤーのインスタンス
	Player* player_ = nullptr;

	uint32_t textureHandle_ = 0;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
};