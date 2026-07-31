#pragma once
#include "Kamataengine.h"
#include "PlayerBullet.h"
#include "3d/DebugCamera.h"

/// <summary>
/// プレイヤークラス
/// </summary>
class Player {
public:
	// デストラクタ
	~Player();

	// 初期化
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);

	// 更新
	void Update();

	// 描画
	void Draw(KamataEngine::Camera* camera);

private:
	/// <summary>
	/// 攻撃
	/// </summary>
	void Attack();

private:
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Model* model_ = nullptr;

	uint32_t textureHandle_ = 0u;

	// キーボード入力
	KamataEngine::Input* input_ = nullptr;

	// 弾
	PlayerBullet* bullet_ = nullptr;
};