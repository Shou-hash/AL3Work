#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>

class MapChipField;
class Player;

enum class BossEnemyLRDirection {
	kLeft,
	kRight,
};

// ★ ボスの行動状態
enum class BossState {
	kWalk,             // 通常移動
	kAttackCharge,     // 突進攻撃溜め（プレイヤーの方向を向く）
	kAttackDash,       // 突進攻撃（その方向へ移動）
	kAttackSlamCharge, // 両手叩きつけ溜め（両手を高く振り上げる）
	kAttackSlam,       // 両手叩きつけ攻撃（両手を振り下ろす）
	kAttackRecoil,     // 攻撃余韻（硬直）
};

class BossEnemy : public BaseEnemy {
public:
	BossEnemy() = default;
	~BossEnemy() override;

	void Initialize(
	    KamataEngine::Model* modelBody, KamataEngine::Model* modelHead, KamataEngine::Model* modelLeft, KamataEngine::Model* modelRight, KamataEngine::Camera* camera,
	    const KamataEngine::Vector3& position, MapChipField* mapChipField);

	void Update() override;
	void Draw() override;
	void OnCollision(Player* player) override;
	void OnDead() override;
	AABB GetAABB() const override;

	int32_t GetHp() const { return hp_; }

	// ★ プレイヤーポインタの設定
	void SetPlayer(Player* player) { player_ = player; }

	// ★ 攻撃中（無敵状態）かどうか
	bool IsAttacking() const { return state_ != BossState::kWalk; }

	// ★ 行動状態の取得
	BossState GetState() const { return state_; }

private:
	KamataEngine::Matrix4x4 MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b);

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	Player* player_ = nullptr; // ★ プレイヤーポインタ

	// 4部位のモデル
	KamataEngine::Model* modelBody_ = nullptr;
	KamataEngine::Model* modelHead_ = nullptr;
	KamataEngine::Model* modelLeft_ = nullptr;
	KamataEngine::Model* modelRight_ = nullptr;

	// 4部位のWorldTransform
	KamataEngine::WorldTransform worldTransformBody_;
	KamataEngine::WorldTransform worldTransformHead_;
	KamataEngine::WorldTransform worldTransformLeft_;
	KamataEngine::WorldTransform worldTransformRight_;

	float animTimer_ = 0.0f;

	// ボス用HPおよびクールダウン時間
	int32_t hp_ = 5;
	int32_t maxHp_ = 5;
	float damageCooldown_ = 0.0f;

	// 歩行移動および攻撃関連
	BossEnemyLRDirection lrDirection_ = BossEnemyLRDirection::kLeft;
	BossState state_ = BossState::kWalk; // ★ 現在の行動状態

	float attackCooldown_ = 0.0f; // ★ 攻撃のクールタイム
	float attackTimer_ = 0.0f;    // ★ 攻撃動作のタイマー

	static inline const float kWalkspeed = 0.03f;
	static inline const float kDashSpeed = 0.42f; // ★ 突進攻撃速度

	// ★ 攻撃制御パラメータ設定
	static inline const float kAttackSearchDistanceX = 6.0f;  // 攻撃検知X距離
	static inline const float kAttackSearchDistanceY = 3.0f;  // 攻撃検知Y距離
	static inline const float kChargeDuration = 0.5f;         // 溜め時間（秒）
	static inline const float kDashDuration = 0.8f;           // 突進時間（秒）
	static inline const float kSlamChargeDuration = 0.6f;     // 叩きつけ溜め時間（秒）
	static inline const float kSlamDuration = 0.4f;           // 叩きつけ時間（秒）
	static inline const float kRecoilDuration = 0.5f;         // 余韻時間（秒）
	static inline const float kAttackCooldownDuration = 2.5f; // 攻撃クールダウン（秒）

	// HPバー表示用スプライト
	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* spriteHpBG_ = nullptr;
	KamataEngine::Sprite* spriteHpBar_ = nullptr;
};