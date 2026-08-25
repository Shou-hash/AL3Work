#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <list>
#include <optional>
#include <vector>

class MapChipField;
class CameraController;
class PlayerHp; // ★ 追加

enum class LRDirection {
	kLeft,
	kRight,
};

enum class Behavior {
	kRoot,       // 通常行動
	kAttack,     // 攻撃行動
	kKnockback,  // ノックバック状態
	kHammerSkill // ★ハンマースキルを追加
};

enum class AttackPhase {
	kCharge, // 溜め
	kDash,   // 突進
	kRecoil, // 余韻
};

class Player {
public:
	friend class GameScene;

	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	struct CollisionMapInfo {
		bool ceilingCollision = false;
		bool onGround = false;
		bool wallCollision = false;
		KamataEngine::Vector3 moveAmount;
	};

	struct HitEffect {
		KamataEngine::WorldTransform worldTransform;
		uint32_t timer = 0;
		uint32_t duration = 20;
		bool isDead = false;
		LRDirection direction = LRDirection::kRight;
	};

	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	AABB GetAABB() const;
	std::optional<AABB> GetAttackAABB() const;

	Player();
	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void KeysPush();
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	void SetCameraController(CameraController* cameraController) { cameraController_ = cameraController; }
	void SetPlayerHp(PlayerHp* playerHp) { playerHp_ = playerHp; } // ★ 追加

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 各メッシュの調整用WorldTransformを取得する関数
	std::vector<KamataEngine::WorldTransform>& GetMeshWorldTransforms() { return meshWorldTransforms_; }

	// ★ 各部位の個別WorldTransformを取得する関数を追加
	KamataEngine::WorldTransform& GetWorldTransformHead() { return worldTransformHead_; }
	KamataEngine::WorldTransform& GetWorldTransformBody() { return worldTransformBody_; }
	KamataEngine::WorldTransform& GetWorldTransformLeft() { return worldTransformLeft_; }
	KamataEngine::WorldTransform& GetWorldTransformRight() { return worldTransformRight_; }

	bool IsDead() const { return isDead_; }
	// ★ハンマースキルの振り下ろしタイミング（進捗0.4以上）も攻撃中として判定するよう修正
	bool IsAttacking() const {
		return (behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kDash) || (behavior_ == Behavior::kHammerSkill && (hammerSkillTimer_ / kHammerSkillDuration) >= 0.4f);
	}
	LRDirection GetLRDirection() const { return lrDirection_; }

	Behavior GetBehavior() const { return behavior_; }

	void OnCollision();
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

	void CreateHitEffect(const KamataEngine::Vector3& position);
	void RequestKnockback() { isKnockbackRequested_ = true; }
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();

	// Playerクラスのpublicメンバに追加
	void SetModelHammer(KamataEngine::Model* model) { modelHammer_ = model; }

	// スキル用初期化 & 更新関数の宣言
	void BehaviorHammerSkillInit();
	void BehaviorHammerSkillUpdate();

	// 調整項目の登録・反映用の static 関数
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

private:
	Behavior behavior_ = Behavior::kRoot;
	std::optional<Behavior> behaviorRequest_ = std::nullopt;

	float EaseOut(float start, float end, float t) {
		float easing = 1.0f - std::pow(1.0f - t, 3.0f);
		return start + (end - start) * easing;
	}

	float EaseIn(float start, float end, float t) {
		float easing = std::pow(t, 3.0f);
		return start + (end - start) * easing;
	}

	// 調整項目（GlobalVariables によって外部から変動可能）
	static inline float kAcceleration = 0.03f;
	static inline float kAttenuation = 0.5f;
	static inline float kLimitRunSpeed = 0.5f;
	static inline float kTimeTurn = 0.8f;
	static inline float kGravityAcceleration = 0.08f;
	static inline float kLimitFallSpeed = 1.0f;
	static inline float kJumpAcceleration = 1.0f;
	static inline float kAttenuationLanding = 0.2f;
	static inline float kGroundSearchOffset = 0.01f;

	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;
	PlayerHp* playerHp_ = nullptr; // ★ 追加
	static inline float kPaddingTop = 1.2f;
	static inline float kPaddingBottom = 0.4f;
	static inline float kPaddingLeft = 0.4f;
	static inline float kPaddingRight = 0.4f;
	KamataEngine::Vector3 velocity_ = {};
	bool onGround_ = true;
	bool isDead_ = false;
	bool isKnockbackRequested_ = false;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	// 各Meshを単独で調整するためのWorldTransform配列
	std::vector<KamataEngine::WorldTransform> meshWorldTransforms_;

	// 各部位の個別モデルポインタ
	KamataEngine::Model* modelPlayerHead_ = nullptr;
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerLeft_ = nullptr;
	KamataEngine::Model* modelPlayerRight_ = nullptr;

	// 各部位の個別WorldTransform (0: Head, 1: Body, 2: Left, 3: Right)
	KamataEngine::WorldTransform worldTransformHead_;
	KamataEngine::WorldTransform worldTransformBody_;
	KamataEngine::WorldTransform worldTransformLeft_;
	KamataEngine::WorldTransform worldTransformRight_;

	LRDirection lrDirection_ = LRDirection::kRight;

	AttackPhase attackPhase_ = AttackPhase::kCharge;
	uint32_t attackParameter_ = 0;

	static inline uint32_t kChargeDuration = 8;
	static inline uint32_t kDashDuration = 8;
	static inline uint32_t kRecoilDuration = 10;
	static inline float kAttackVelocity = 0.85f;

	KamataEngine::Model* modelHitEffect_ = nullptr;
	std::list<HitEffect*> hitEffects_;

	float knockbackTimer_ = 0.0f;
	static inline float kKnockbackSpeedDuration = 0.2f;
	static inline float kKnockbackTotalDuration = 0.5f;

	// 歩きアニメーション用のタイマー変数
	float walkAnimationTimer_ = 0.0f;

	// Playerクラスのprivateメンバに追加
	KamataEngine::Model* modelHammer_ = nullptr;
	KamataEngine::WorldTransform worldTransformHammer_;

	// アニメーション制御用タイマーとフラグ
	float hammerSkillTimer_ = 0.0f;
	bool isHammerVisible_ = false;
	static inline float kHammerSkillDuration = 0.6f; // スキル全体の時間（秒）
};