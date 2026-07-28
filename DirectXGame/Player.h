#pragma once
#include "BaseEnemy.h" // ★ Enemy.h ではなく BaseEnemy.h をインクルード
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <list>
#include <optional>

class MapChipField;
class CameraController;

enum class LRDirection {
	kLeft,
	kRight,
};

enum class Behavior {
	kRoot,     // 通常行動
	kAttack,   // 攻撃行動
	kKnockback // ノックバック状態
};

enum class AttackPhase {
	kCharge, // 溜め
	kDash,   // 突進
	kRecoil, // 余韻
};

class Player {
public:
	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	struct CollisionMapInfo {
		bool ceilingCollision = false;
		bool onGround = false;
		bool wallCollision = false;
		KamataEngine::Vector3 moveAmount;
	};

	// ヒットエフェクト管理用の構造体
	struct HitEffect {
		KamataEngine::WorldTransform worldTransform;
		uint32_t timer = 0;
		uint32_t duration = 20; // エフェクトの生存フレーム数
		bool isDead = false;
		LRDirection direction = LRDirection::kRight;
	};

	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	AABB GetAABB() const;

	// 攻撃用のAABBを取得する関数
	std::optional<AABB> GetAttackAABB() const;

	Player();
	~Player(); // デストラクタで残ったエフェクトやモデルを破棄します

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void KeysPush();
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	void SetCameraController(CameraController* cameraController) { cameraController_ = cameraController; }

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 死亡状態の取得
	bool IsDead() const { return isDead_; }

	// 攻撃中（突進中）かどうかの取得
	bool IsAttacking() const { return behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kDash; }

	// 向きを取得するGetter
	LRDirection GetLRDirection() const { return lrDirection_; }

	// 衝突時に呼び出される関数（デスフラグを立てる）
	void OnCollision();

	// ★ 敵との衝突判定（引数を BaseEnemy のリストに変更）
	void CheckEnemyCollision(const std::list<BaseEnemy*>& enemies);

	void BehaviorRootUpdate();
	void BehaviorAttackUpdate();

	void BehaviorRootInit();
	void BehaviorAttackInit();

	void Move();
	void MapCollision(CollisionMapInfo& info);
	void MapCollisionTop(CollisionMapInfo& info);
	void MapCollisionBottom(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);
	void ApplyGroundingStatus(const CollisionMapInfo& info);
	void CheckScreenEdgeCollision();
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

	// エフェクト発生用の関数
	void CreateHitEffect(const KamataEngine::Vector3& position);

	// ノックバックリクエストを受ける関数
	void RequestKnockback() { isKnockbackRequested_ = true; }
	// ノックバック用の更新関数・初期化関数
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();

private:
	Behavior behavior_ = Behavior::kRoot;
	std::optional<Behavior> behaviorRequest_ = std::nullopt;

	// だんだん減速する（EaseOut）
	float EaseOut(float start, float end, float t) {
		float easing = 1.0f - std::pow(1.0f - t, 3.0f);
		return start + (end - start) * easing;
	}

	// だんだん加速する（EaseIn）
	float EaseIn(float start, float end, float t) {
		float easing = std::pow(t, 3.0f);
		return start + (end - start) * easing;
	}

	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.5f;
	static inline const float kLimitRunSpeed = 2.0f;
	static inline const float kTimeTurn = 0.8f;
	static inline const float kGravityAcceleration = 0.08f;
	static inline const float kLimitFallSpeed = 1.0f;
	static inline const float kJumpAcceleration = 1.0f;
	static inline const float kAttenuationLanding = 0.2f;
	static inline const float kGroundSearchOffset = 0.01f;

	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;
	KamataEngine::Vector3 velocity_ = {};
	bool onGround_ = true;
	bool isDead_ = false; // 死亡フラグ
	bool isKnockbackRequested_ = false;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	LRDirection lrDirection_ = LRDirection::kRight;

	// 攻撃行動用（サブフェーズ定義）
	AttackPhase attackPhase_ = AttackPhase::kCharge; // 現在の攻撃フェーズ
	uint32_t attackParameter_ = 0;                   // 各フェーズの進捗タイマー

	static inline const uint32_t kChargeDuration = 8;  // 溜め動作時間
	static inline const uint32_t kDashDuration = 8;    // 突進動作時間
	static inline const uint32_t kRecoilDuration = 10; // 余韻動作時間

	static inline const float kAttackVelocity = 0.85f; // 攻撃突進時の移動速度

	// ヒットエフェクト用のメンバ変数群
	KamataEngine::Model* modelHitEffect_ = nullptr; // エフェクトのモデルポインタ
	std::list<HitEffect*> hitEffects_;

	// ノックバック演出用タイマーとフェーズ管理
	float knockbackTimer_ = 0.0f;
	static inline const float kKnockbackSpeedDuration = 0.2f; // 「強い初速で弾き飛ばされる」時間
	static inline const float kKnockbackTotalDuration = 0.5f; // ノックバック全体の時間
};