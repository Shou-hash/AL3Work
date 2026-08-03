#pragma once
#include "3d/AxisIndicator.h"
#include "3d/DebugCamera.h"
#include <KamataEngine.h>

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	// 行動フェーズ
	enum class Phase {
		Approach, // 接近する
		Leave,    // 離脱する
	};

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデルのポインタ</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	/// <param name="camera">カメラ</param>
	void Draw(const KamataEngine::Camera& camera);

	// デストラクタ
	~Enemy();

private:
	// フェーズ毎の更新処理
	void ApproachUpdate();
	void LeaveUpdate();

private:
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// モデルのポインタ
	KamataEngine::Model* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// フェーズ（初期フェーズ：接近）
	Phase phase_ = Phase::Approach;

	// メンバ関数ポインタテーブルの宣言
	static void (Enemy::* staticFunctionTable[])();
};