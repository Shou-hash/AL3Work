#pragma once
#include "3d/DebugCamera.h"
#include "KamataEngine.h"
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

private:
	// カメラ
	KamataEngine::Camera camera_;

	// プレイヤーのインスタンス
	Player* player_ = nullptr;

	uint32_t textureHandle_ = 0;
	KamataEngine::Model* model_ = nullptr;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// デバッグカメラ有効フラグ
	bool isDebugCameraActive_ = false;

	// キー入力取得用
	KamataEngine::Input* input_ = nullptr;
};