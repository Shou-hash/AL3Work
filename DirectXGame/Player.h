#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <list>
#include <optional>

class MapChipField;
class CameraController;
class Enemy;

enum class LRDirection {
	kLeft,
	kRight,
};

enum class Behavior {
	kRoot,   // 通常行動
	kAttack, // 攻撃行動
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

	// ★ ヒートエフェクト管理用の構造体
	struct HitEffect {
		KamataEngine::WorldTransform worldTransform;
		uint32_t timer = 0;
		uint32_t duration = 20; // エフェクトの生存フレーム数
		bool isDead = false;
	};

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

	// 衝突時に呼び出される関数（デスフラグを立てる）
	void OnCollision();

	// 敵との衝突判定
	void CheckEnemyCollision(const std::list<Enemy*>& enemies);

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

private:
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

	// 状態管理用の変数群
	Behavior behavior_ = Behavior::kRoot;                    // 現在のビヘイビア
	std::optional<Behavior> behaviorRequest_ = std::nullopt; // 状態遷移へのリクエスト

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

	// ヒートエフェクト用のメンバ変数群
	KamataEngine::Model* modelHitEffect_ = nullptr; // エフェクトのモデルポインタ
	std::list<HitEffect*> hitEffects_;
};