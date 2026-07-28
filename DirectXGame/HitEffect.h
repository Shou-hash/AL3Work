#pragma once
#include "BaseEffect.h"
#include <3d/WorldTransform.h>
#include <KamataEngine.h>

class HitEffect : public BaseEffect {
public:
	// 静的メンバ変数のセッター (静的関数)
	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="position">発生させる座標</param>
	void Initialize(const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// デスフラグ（終了判定）の取得
	/// </summary>
	bool IsFinished() const override { return isFinished_; }

private:
	// 通常エフェクト用のトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// 楕円リング用のトランスフォーム
	KamataEngine::WorldTransform worldTransformRing_;

	// 各エフェクトの最大スケール倍率（乱数で決定）
	float ringMaxScale_ = 4.0f;
	float normalMaxScale_ = 2.0f;

	// 各エフェクトの色（フェードアウト用にアルファ値をコントロールする）
	KamataEngine::Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	KamataEngine::Vector4 colorRing_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// 生存タイマー
	float timer_ = 0.0f;
	static inline const float kDuration = 0.5f; // 表示する長さ（秒）
	bool isFinished_ = false;

private:
	// 静的メンバ変数 (クラス全体で共有)
	static KamataEngine::Model* model_;
	static KamataEngine::Camera* camera_;
};