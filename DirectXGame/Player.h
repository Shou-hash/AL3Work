#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include "PlayerAnimation.h"
#include <3d/WorldTransform.h>
#include <array>
#include <list>
#include <optional>
#include <vector>

class MapChipField;
class CameraController;
class PlayerHp;     // ★ 追加
class LightManager; // ★ 追加：ライティング設定用
struct BlockData;   // ★ 追加: 前方宣言

enum class LRDirection {
	kLeft,
	kRight,
};

enum class Behavior {
	kRoot,        // 通常行動
	kAttack,      // 攻撃行動
	kKnockback,   // ノックバック状態
	kHammerSkill, // ハンマースキルを追加
	kDeath        // 死亡アニメーション状態
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

	// ★ 追加: 雲マップチップ判定用データ構造体
	struct CloudBlockInfo {
		KamataEngine::Vector3 position;
		float minX;
		float maxX;
		float minY;
		float maxY;
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
	void SetPlayerHp(PlayerHp* playerHp) { playerHp_ = playerHp; }

	// ★ 追加: 雲データ参照の設定関数
	void SetCloudDatas(std::vector<BlockData*>* cloudDatas) { cloudDatas_ = cloudDatas; }

	// ★ 追加: 接地状態の取得関数
	bool IsOnGround() const { return onGround_; }

	// ★ 追加: Y座標の加算関数（雲の揺れ追従用）
	void AddPositionY(float deltaY) { worldTransform_.translation_.y += deltaY; }

	// ★ 追加: 足元の雲に衝撃を与える関数
	void ApplyCloudImpulse(float impulse);

	// ★ プレイヤーのモデルにも、マップタイルなどと同じライトグループ（平行光源・太陽の回転など）を設定する
	void ApplyLightManager(const LightManager& lightManager);

	// ★ 死亡アニメーション演出が完了したか判定する関数
	bool IsDeathAnimationFinished() const { return isDeathAnimationFinished_; }

	void BehaviorDeathInit();   // ★追加
	void BehaviorDeathUpdate(); // ★追加

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 各メッシュの調整用WorldTransformを取得する関数
	std::vector<KamataEngine::WorldTransform>& GetMeshWorldTransforms() { return meshWorldTransforms_; }

	// ★ 各部位の個別WorldTransformを取得する関数を追加
	KamataEngine::WorldTransform& GetWorldTransformHead() { return animation_.GetWorldTransformHead(); }
	KamataEngine::WorldTransform& GetWorldTransformBody() { return animation_.GetWorldTransformBody(); }
	KamataEngine::WorldTransform& GetWorldTransformLeft() { return animation_.GetWorldTransformLeft(); }
	KamataEngine::WorldTransform& GetWorldTransformRight() { return animation_.GetWorldTransformRight(); }

	bool IsDead() const { return isDead_; }
	// ★攻撃判定の修正：ハンマー攻撃中も攻撃中として判定
	bool IsAttacking() const { return (behavior_ == Behavior::kAttack && animation_.GetAttackPhase() == AttackPhase::kDash) || (behavior_ == Behavior::kHammerSkill); }

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

	// ★ 追加: 周辺の雲マップチップ情報を取得する関数
	std::vector<CloudBlockInfo> GetNearbyClouds(const KamataEngine::Vector3& pos) const;

	void CreateHitEffect(const KamataEngine::Vector3& position);
	void RequestKnockback() { isKnockbackRequested_ = true; }
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();

	// Playerクラスのpublicメンバに追加
	void SetModelHammer(KamataEngine::Model* model) { modelHammer_ = model; }

	// スキル用初期化 & 更新関数の宣言
	void BehaviorHammerSkillInit();
	void BehaviorHammerSkillUpdate();

	// ★ 処刑演出（中ボス撃破後の連撃）用
	void BeginExecution(const KamataEngine::Vector3& position, LRDirection direction);
	void EndExecution();
	bool IsExecuting() const { return isExecuting_; }
	bool StartExecutionSwing(uint32_t comboIndex);
	bool IsExecutionSwinging() const { return executionSwinging_; }
	bool ConsumeExecutionHit() {
		bool hit = executionHitPending_;
		executionHitPending_ = false;
		return hit;
	}

	// 調整項目の登録・反映用の static 関数
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

private:
	Behavior behavior_ = Behavior::kRoot;
	std::optional<Behavior> behaviorRequest_ = std::nullopt;

	// 調整項目（GlobalVariables によって外部から変動可能）
	static inline float kAcceleration = 0.015f;
	static inline float kAttenuation = 0.4f;
	static inline float kLimitRunSpeed = 0.025f;
	static inline float kTimeTurn = 0.8f;
	static inline float kGravityAcceleration = 0.08f;
	static inline float kLimitFallSpeed = 1.0f;
	static inline float kJumpAcceleration = 1.0f;
	static inline float kAttenuationLanding = 0.2f;
	static inline float kGroundSearchOffset = 0.01f;

	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;
	PlayerHp* playerHp_ = nullptr; // ★ 追加
	static inline float kPaddingTop = 2.0f;
	static inline float kPaddingBottom = 0.1f;
	static inline float kPaddingLeft = 0.4f;
	static inline float kPaddingRight = 0.4f;
	KamataEngine::Vector3 velocity_ = {};
	bool onGround_ = true;
	bool isDead_ = false;
	bool isKnockbackRequested_ = false;

	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	// 各Meshを単独で調整するためのWorldTransform配列
	std::vector<KamataEngine::WorldTransform> meshWorldTransforms_;

	// プレイヤーアニメーション管理クラス
	PlayerAnimation animation_;

	LRDirection lrDirection_ = LRDirection::kRight;

	static inline float kAttackVelocity = 0.4f;

	KamataEngine::Model* modelHitEffect_ = nullptr;
	std::list<HitEffect*> hitEffects_;

	float knockbackTimer_ = 0.0f;
	static inline float kKnockbackSpeedDuration = 0.2f;
	static inline float kKnockbackTotalDuration = 0.5f;

	// Playerクラスのprivateメンバに追加
	KamataEngine::Model* modelHammer_ = nullptr;

	bool isDeathAnimationFinished_ = false; // ★追加

	// ★ ハンマー連撃用管理変数
	uint32_t hammerComboIndex_ = 0;
	bool comboInputRequested_ = false;

	// ★ 処刑演出用
	void UpdateExecution();
	bool isExecuting_ = false;         // 処刑演出中
	bool executionSwinging_ = false;   // ハンマーのスイング中
	bool executionHitPending_ = false; // スイングが当たったタイミング（ConsumeExecutionHit で取得される）
	bool executionHitDone_ = false;    // このスイングで当たりを通知済みか

	std::vector<BlockData*>* cloudDatas_ = nullptr; // ★ 追加: 雲データ参照ポインタ
	float prevVelocityY_ = 0.0f;                    // ★ 追加: 前フレームのY速度（着地衝撃計算用）
	bool wasOnGround_ = false;                      // ★ 追加: 前フレームの接地フラグ
};