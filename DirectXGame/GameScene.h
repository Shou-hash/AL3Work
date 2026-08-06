#pragma once
#include "3d/DebugCamera.h"
#include "Enemy.h"
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

	/// <summary>
	/// 衝突判定と応答
	/// </summary>
	void CheckAllCollisions();

private:
	// 敵クラスのポインタを保持
	Enemy* enemy_ = nullptr;

	// 敵用モデルとテクスチャ
	KamataEngine::Model* enemyModel_ = nullptr;
	uint32_t enemyTextureHandle_ = 0;

	// カメラ
	KamataEngine::Camera camera_;

	// プレイヤーのインスタンス
	Player* player_ = nullptr;

	uint32_t playerTex_ = 0;
	KamataEngine::Model* model_ = nullptr;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// デバッグカメラ有効フラグ
	bool isDebugCameraActive_ = false;

	// キー入力取得用
	KamataEngine::Input* input_ = nullptr;
};