#pragma once
#include "3d/AxisIndicator.h"
#include "3d/DebugCamera.h"
#include "EnemyBullet.h"
#include <KamataEngine.h>
#include <list>

// 前方宣言
class BaseEnemyState;

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	// 発射間隔 (60frame = 1秒ごとに発射)
	static const int kFireInterval = 60;

	// デストラクタ
	~Enemy();

	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	void ChangeState(BaseEnemyState* newState);

	/// <summary>
	/// 接近フェーズ初期化
	/// </summary>
	void ApproachInitialize();

	/// <summary>
	/// 接近フェーズ更新
	/// </summary>
	void ApproachUpdate();

	/// <summary>
	/// 弾発射
	/// </summary>
	void Fire();

	// ゲッター
	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// モデルのポインタ
	KamataEngine::Model* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// 現在のステート
	BaseEnemyState* state_ = nullptr;

	// --- 敵の弾関係 ---
	// 敵弾のリスト（複数管理）
	std::list<EnemyBullet*> bullets_;

	// 発射タイマー
	int32_t fireTimer_ = 0;
};