#pragma once
#include "3d/Model.h"
#include "3d/WorldTransform.h"
#include "KamataEngine.h"

// 前方宣言
class Player;

/// <summary>
/// 敵の弾
/// </summary>
class EnemyBullet {
public:
	static const int32_t kLifeTime = 60 * 5;

	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void Update();
	void Draw(const KamataEngine::Camera& camera);

	bool IsDead() const { return isDead_; }

	// 自機（Player）のポインタをセットする関数
	void SetPlayer(Player* player) { player_ = player; }

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// 速度
	KamataEngine::Vector3 velocity_;

	// デスタイマー
	int32_t deathTimer_ = kLifeTime;

	// デスフラグ
	bool isDead_ = false;

	// 自機ポインタ
	Player* player_ = nullptr;
};