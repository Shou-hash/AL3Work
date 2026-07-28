#pragma once
#include "BaseEnemy.h"

class Player;

/// <summary>
/// 基本の敵
/// </summary>
class Enemy final : public BaseEnemy {
public:
	enum class Behavior {
		kRoot, // 通常状態
		kDead, // 死亡状態
	};

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	// オーバーライド
	void Initialize() override {}
	void Update() override;
	void Draw() override;
	void OnDead() override;
	void OnCollision(Player* player) override;
	AABB GetAABB() const override; // ★追加：GetAABB の宣言

	// 通常状態の更新
	void BehaviorRootUpdate();
	// 死亡状態の更新
	void BehaviorDeadUpdate();

private:
	// 描画に必要なエンジン系のポインタ
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;

	KamataEngine::Vector3 velocity_ = {0, 0, 0}; // 敵の現在の速度

	static inline const float kWalkspeed = 0.05f; // 敵の移動速度
	static inline const float kWalkMotionAnglestart = -20.0f;
	static inline const float kWalkMotionAngleEnd = 30.0f;
	static inline const float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f; // 歩行モーションのタイマー

	Behavior behavior_ = Behavior::kRoot;           // 現在の状態
	float deadTimer_ = 0.0f;                        // 死亡アニメーション用タイマー
	static inline const float kDeadDuration = 1.0f; // 死亡演出の長さ（秒）
};