#pragma once
#include <KamataEngine.h>
#include "3d/DebugCamera.h"
#include "3d/AxisIndicator.h"

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
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
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// モデルのポインタ
	KamataEngine::Model* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// 移動速度（Z軸方向）
	float speed_ = -0.5f; // 手前に進める場合はマイナス（奥に行く場合はプラスに変更してください）
};