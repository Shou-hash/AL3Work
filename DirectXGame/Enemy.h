#pragma once
#include "3d/AxisIndicator.h"
#include "3d/DebugCamera.h"
#include <KamataEngine.h>

// 前方宣言
class BaseEnemyState;

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const KamataEngine::Camera& camera);

	/// <summary>
	/// ステートの変更
	/// </summary>
	void ChangeState(BaseEnemyState* newState);

	// デストラクタ
	~Enemy();

	//（Stateクラスから参照・操作用）
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
};