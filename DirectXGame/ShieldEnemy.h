#pragma once
#include "BaseEnemy.h"
#include <list>

class Player;

enum class ShieldEnemyLRDirection {
	kLeft,
	kRight,
};

/// <summary>
/// 盾の敵
/// </summary>
class ShieldEnemy final : public BaseEnemy {
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

	// 初期化・更新・描画
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update() override;
	void Draw() override;
	void OnDead() override;
	void OnCollision(Player* player) override;
	AABB GetAABB() const override; // ★追加：GetAABB の宣言

	// 各状態の更新
	void BehaviorRootUpdate();
	void BehaviorDeadUpdate();
	void BehaviorGuardUpdate();

	// ガードエフェクト生成
	void CreateGuardEffect();

	static void StaticFinalize();

	// ゲッター
	ShieldEnemyLRDirection GetLRDirection() const { return lrDirection_; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;

	// ガードモデル用
	static KamataEngine::Model* modelGuardEffect_;
	std::list<GuardEffect*> guardEffects_;

	ShieldEnemyLRDirection lrDirection_ = ShieldEnemyLRDirection::kLeft;
	KamataEngine::Vector3 velocity_ = {0.0f, 0.0f, 0.0f};

	static inline const float kWalkspeed = 0.05f;
	static inline const float kWalkMotionAnglestart = -20.0f;
	static inline const float kWalkMotionAngleEnd = 30.0f;
	static inline const float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f;

	Behavior behavior_ = Behavior::kRoot;

	// デス用タイマー
	float deadTimer_ = 0.0f;
	static inline const float kDeadDuration = 1.0f;

	// ガード（のけぞり）用タイマー・パラメータ
	float guardTimer_ = 0.0f;
	static inline const float kGuardDuration = 0.3f; // ガードアニメーションの時間（秒）
	float baseRotationX_ = 0.0f;                     // 元のX軸回転量（基本は0）
};