#pragma once
#include "3d/Model.h"
#include "3d/WorldTransform.h"
#include "KamataEngine.h"

/// <summary>
/// 自キャラの弾
/// </summary>
class PlayerBullet {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(); // ← 引数なしに戻します

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const KamataEngine::Camera& camera);

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;
};