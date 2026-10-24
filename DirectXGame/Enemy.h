#pragma once
#include "BaseEnemy.h"

class Player;
class MapChipField; // ★追加：MapChipFieldの不完全宣言

class Enemy final : public BaseEnemy {
public:
	enum class Behavior {
		kRoot,
		kDead,
		kComboHit, // ★ ハンマーの連撃を受けている状態（その場で止まり、打撃のたびにのけぞる）
	};

	// ★変更：MapChipFieldのポインタを追加
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField);

	void Initialize() override {}
	void Update() override;
	void Draw() override;
	void OnDead() override;
	void OnCollision(Player* player) override;
	AABB GetAABB() const override;

	void BehaviorRootUpdate();
	void BehaviorDeadUpdate();
	void BehaviorComboHitUpdate(); // ★ 追加: 連撃を受けている間の更新

	// ★ ハンマーの連撃を 1 回受ける（swingId: プレイヤーのスイング番号。同じスイングでの多重ヒットは無視して false を返す）
	//   連撃を受けている間は敵の行動が止まり、規定回数（kHammerHitsToDefeat）に達すると吹き飛んで倒れる。連撃が途切れたら通常行動に戻る
	bool ReceiveHammerHit(uint32_t swingId, float pushDirX);

	// ★ 連撃を受けて止まっている状態かどうか（この間は触れてもプレイヤーはダメージを受けない）
	bool IsComboHitState() const { return behavior_ == Behavior::kComboHit; }
	int32_t GetComboHitCount() const { return comboHitCount_; }

	// 調整項目の登録・反映用の static 関数
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

	// アイテムドロップ状態（死亡演出中）かどうかの判定
	bool IsItemState() const { return behavior_ == Behavior::kDead; }

	// ★追加：アイテム生成要求のチェックとリセット
	bool IsItemSpawnRequested() const { return isItemSpawnRequested_; }
	void ResetItemSpawnRequest() { isItemSpawnRequested_ = false; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	MapChipField* mapChipField_ = nullptr; // ★追加：メンバー変数の保持

	KamataEngine::Vector3 velocity_ = {0, 0, 0};

	static inline float kWalkspeed = 0.05f;
	static inline float kWalkMotionAnglestart = -20.0f;
	static inline float kWalkMotionAngleEnd = 30.0f;
	static inline float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f;

	Behavior behavior_ = Behavior::kRoot;
	float deadTimer_ = 0.0f;
	static inline float kDeadDuration = 1.0f;

	bool isItemSpawnRequested_ = false; // ★追加：アイテムドロップ要求フラグ

	// ★ ハンマー連撃用
	int32_t comboHitCount_ = 0;      // 現在の連撃を受けた回数
	uint32_t lastHammerSwingId_ = 0; // 最後に打撃を受けたスイング番号（同じスイングでの多重ヒット防止）
	float comboStunTimer_ = 0.0f;    // 最後の打撃からの経過時間（連撃が途切れたら通常行動へ戻る）
	float hitReactionTimer_ = 0.0f;  // 打撃 1 回分ののけぞりの経過時間
	float hitPushDirX_ = 1.0f;       // 打撃で押される向き（+1 右 / -1 左）

	static inline const int32_t kHammerHitsToDefeat = 3;    // 倒れるまでに必要な連撃の回数
	static inline const float kHitReactionDuration = 0.25f; // 打撃 1 回分ののけぞりの長さ（秒）
	static inline const float kComboRecoverTime = 1.2f;     // 連撃が途切れてから通常行動へ戻るまでの時間（秒）
};
