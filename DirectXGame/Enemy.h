#pragma once
#include "3d/AxisIndicator.h"
#include "3d/DebugCamera.h"
#include "EnemyBullet.h"
#include "TimedCall.h"
#include <KamataEngine.h>
#include <functional>
#include <list>

// 前方宣言
class BaseEnemyState;

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	// 発射間隔 (60frame = 1秒ごとに発射)
	static const uint32_t kFireInterval = 60;

	// デストラクタ
	~Enemy();

	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	void ChangeState(BaseEnemyState* newState);

	/// <summary>
	/// 接近フェーズ初期化処理
	/// </summary>
	void InitializeApproachPhase();

	/// <summary>
	/// 弾を発射し、次のタイマーをリセット(予約)するコールバック関数
	/// </summary>
	void FireAndReset();

	/// <summary>
	/// 登録済みの時限発動イベントをクリアする処理
	/// </summary>
	void ClearTimedCalls();

	/// <summary>
	/// 弾発射の実体関数
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

	// 敵弾のリスト
	std::list<EnemyBullet*> bullets_;

	// 時限発動イベントのリスト
	std::list<TimedCall*> timedCalls_;
};