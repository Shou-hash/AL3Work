#pragma once
#include "3d/Model.h"
#include "3d/WorldTransform.h"
#include "KamataEngine.h"

/// <summary>
/// 敵の弾
/// </summary>
class EnemyBullet {
public:
	// 寿命<frm>
	static const int32_t kLifeTime = 60 * 5;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const KamataEngine::Camera& camera);

	// デスフラグの getter
	bool IsDead() const { return isDead_; }

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
};