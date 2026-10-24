#pragma once
#include "BaseEnemy.h"
#include <list>

class Player;
class MapChipField; // ★追加：MapChipFieldの不完全宣言

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
		kRoot,     // 通常（歩行）状態
		kDead,     // デス演出
		kGuard,    // ガードリアクション（のけぞり）
		kComboHit, // ★ ハンマーの連撃を受けている状態（その場で止まり、打撃のたびにのけぞる）
	};

	// ガードエフェクト用構造体
	struct GuardEffect {
		KamataEngine::WorldTransform worldTransform;
		uint32_t timer = 0;
		uint32_t duration = 15; // 表示フレーム数
		bool isDead = false;
	};

	// ★変更：引数に MapChipField のポインタを追加
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField);
	void Update() override;
	void Draw() override;
	void OnDead() override;
	void OnCollision(Player* player) override;
	AABB GetAABB() const override;

	// 各状態の更新
	void BehaviorRootUpdate();
	void BehaviorDeadUpdate();
	void BehaviorGuardUpdate();
	void BehaviorComboHitUpdate(); // ★ 追加: 連撃を受けている間の更新

	// ★ ハンマーの連撃を 1 回受ける（ハンマーは盾のガードを崩して連撃になる。同じスイングでの多重ヒットは無視して false を返す）
	bool ReceiveHammerHit(uint32_t swingId, float pushDirX);

	// ★ 連撃を受けて止まっている状態かどうか（この間は触れてもプレイヤーはダメージを受けない）
	bool IsComboHitState() const { return behavior_ == Behavior::kComboHit; }
	int32_t GetComboHitCount() const { return comboHitCount_; }

	// ガードエフェクト生成
	void CreateGuardEffect();

	static void StaticFinalize();

	// ゲッター
	ShieldEnemyLRDirection GetLRDirection() const { return lrDirection_; }

	// ★追加：アイテムドロップ状態（死亡演出中）かどうかの判定
	bool IsItemState() const { return behavior_ == Behavior::kDead; }

	// ★追加：アイテム生成要求のチェックとリセット
	bool IsItemSpawnRequested() const { return isItemSpawnRequested_; }
	void ResetItemSpawnRequest() { isItemSpawnRequested_ = false; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;
	MapChipField* mapChipField_ = nullptr; // ★追加：メンバー変数の保持

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

	bool isItemSpawnRequested_ = false; // ★追加：アイテムドロップ要求フラグ

	// ★ ハンマー連撃用
	int32_t comboHitCount_ = 0;      // 現在の連撃を受けた回数
	uint32_t lastHammerSwingId_ = 0; // 最後に打撃を受けたスイング番号（同じスイングでの多重ヒット防止）
	float comboStunTimer_ = 0.0f;    // 最後の打撃からの経過時間（連撃が途切れたら通常行動へ戻る）
	float hitReactionTimer_ = 0.0f;  // 打撃 1 回分ののけぞりの経過時間
	float hitPushDirX_ = 1.0f;       // 打撃で押される向き（+1 右 / -1 左）

	static inline const int32_t kHammerHitsToDefeat = 4;    // 倒れるまでに必要な連撃の回数（盾の敵は頑丈）
	static inline const float kHitReactionDuration = 0.25f; // 打撃 1 回分ののけぞりの長さ（秒）
	static inline const float kComboRecoverTime = 1.2f;     // 連撃が途切れてから通常行動へ戻るまでの時間（秒）
};
