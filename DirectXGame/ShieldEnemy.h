#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>
#include <list>

class Player;

enum class ShieldEnemyLRDirection {
	kLeft,
	kRight,
};

class ShieldEnemy {
public:
	~ShieldEnemy();

	enum class Behavior {
		kRoot,  // 通常（歩行）状態
		kDead,  // デス演出
		kGuard, // ガードリアクション（のけぞり）
	};

	// ガードエフェクト用構造体
	struct GuardEffect {
		KamataEngine::WorldTransform worldTransform;
		uint32_t timer = 0;
		uint32_t duration = 15; // 表示フレーム数
		bool isDead = false;
	};

	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	// 初期化・更新・描画
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	// 衝突応答（※ Playerの書き換えを行うため const を除去）
	void OnCollision(Player* player);

	// 各状態の更新
	void BehaviorRootUpdate();
	void BehaviorDeadUpdate();
	void BehaviorGuardUpdate();

	// 死亡演出開始
	void OnDead();

	// ガードエフェクト生成
	void CreateGuardEffect();

	static void StaticFinalize();

	// ゲッター
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	ShieldEnemyLRDirection GetLRDirection() const { return lrDirection_; }
	bool IsDead() const { return isDead_; }
	AABB GetAABB() const;

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;

	// ガードモデル用
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

	// デス用タイマー
	float deadTimer_ = 0.0f;
	static inline const float kDeadDuration = 1.0f;

	// ガード（のけぞり）用タイマー・パラメータ
	float guardTimer_ = 0.0f;
	static inline const float kGuardDuration = 0.3f; // ガードアニメーションの時間（秒）
	float baseRotationX_ = 0.0f;                     // 元のX軸回転量（基本は0）
};