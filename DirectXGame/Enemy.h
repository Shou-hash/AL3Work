#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>

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

	// 初期化関数（override を明記）
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) override;

	// BaseEnemyからのオーバーライド関数
	void Update() override;
	void Draw() override;
	void OnCollision(Player* player) override;
	void OnDead() override;
	bool IsDead() const override { return isDead_; }
	AABB GetAABB() const override;
	const KamataEngine::WorldTransform& GetWorldTransform() const override { return worldTransform_; }

	// 通常状態の更新
	void BehaviorRootUpdate();
	// 死亡状態の更新
	void BehaviorDeadUpdate();

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Vector3 velocity_ = {0, 0, 0};
	static inline const float kWalkspeed = 0.05f;
	static inline const float kWalkMotionAnglestart = -20.0f;
	static inline const float kWalkMotionAngleEnd = 30.0f;
	static inline const float kWalkMotionTime = 1.0f;
	float walkTimer_ = 0.0f;

	Behavior behavior_ = Behavior::kRoot;
	bool isDead_ = false;
	bool isCollisionDisabled_ = false;
	float deadTimer_ = 0.0f;
	static inline const float kDeadDuration = 1.0f;
};