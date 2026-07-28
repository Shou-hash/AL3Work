#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <list>

class Player;

enum class ShieldEnemyLRDirection {
	kLeft,
	kRight,
};

/// <summary>
/// 盾持ちの敵
/// </summary>
class ShieldEnemy final : public BaseEnemy {
public:
	~ShieldEnemy() override;

	enum class Behavior {
		kRoot,  // 通常（歩行）状態
		kDead,  // デス演出
		kGuard, // ガードリアクション（のけぞり）
	};

	struct GuardEffect {
		KamataEngine::WorldTransform worldTransform;
		uint32_t timer = 0;
		uint32_t duration = 15;
		bool isDead = false;
	};

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	// BaseEnemyからのオーバーライド関数
	void Update() override;
	void Draw() override;
	void OnCollision(Player* player) override;
	void OnDead() override;
	bool IsDead() const override { return isDead_; }
	AABB GetAABB() const override;
	const KamataEngine::WorldTransform& GetWorldTransform() const override { return worldTransform_; }

	void BehaviorRootUpdate();
	void BehaviorDeadUpdate();
	void BehaviorGuardUpdate();

	void CreateGuardEffect();
	static void StaticFinalize();

	ShieldEnemyLRDirection GetLRDirection() const { return lrDirection_; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;

	static KamataEngine::Model* modelGuardEffect_;
	std::list<GuardEffect*> guardEffects_;

	ShieldEnemyLRDirection lrDirection_ = ShieldEnemyLRDirection::kLeft;
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_ = {0.0f, 0.0f, 0.0f};

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

	float guardTimer_ = 0.0f;
	static inline const float kGuardDuration = 0.3f;
	float baseRotationX_ = 0.0f;
};