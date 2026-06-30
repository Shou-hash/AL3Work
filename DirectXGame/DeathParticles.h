#pragma once
#include<KamataEngine.h>
#include <array>

/// <summary>
/// デスパーティクル
/// </summary>
class DeathParticles {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="viewProjection">ビュープロジェクション</param>
	/// <param name="position">初期座標</param>
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* viewProjection, const KamataEngine::Vector3& position);	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 終了フラグの取得
	/// </summary>
	bool IsFinished() const { return isFinished_; }

private:
	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;

	// カウントダウンタイマー（1秒 = 60フレーム）
	static inline const uint32_t kDuration = 60;

	bool isInitialized_ = false;

	// モデルのポインタ
	KamataEngine::Model* model_ = nullptr;
	// カメラのポインタ
	KamataEngine::Camera* viewProjection_ = nullptr;

	// ワールドトランスフォーム（固定長配列）
	std::array<KamataEngine::WorldTransform, kNumParticles> worldTransforms_;

	// 速度のメンバ変数（固定長配列）
	std::array<KamataEngine::Vector3, kNumParticles> velocities_;

	// 生存フラグ
	bool isFinished_ = false;

	// タイマーカウンター
	uint32_t counter_ = 0;
};