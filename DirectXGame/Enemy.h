#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>

class Player;

class Enemy {
public:

	enum class Behavior {
		kRoot, // 通常状態
		kDead, // 死亡状態
	};

	// プレイヤーを参考に、モデル・カメラ・初期位置を受け取る初期化関数
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update();
	void Draw();

	// ワールドトランスフォームの取得（当たり判定用）
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	void OnCollision(const Player* player);

	// 通常状態の更新
	void BehaviorRootUpdate();
	// 死亡状態の更新 (追加)
	void BehaviorDeadUpdate();

	// 死亡演出を開始する関数 (追加)
	void OnDead();

	bool IsDead() const { return isDead_; }

	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	AABB GetAABB() const;

private:
	// 描画に必要なエンジン系のポインタ
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;

	// 位置・回転・スケールを管理するワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Vector3 velocity_ = {0, 0, 0}; // 敵の現在の速度

	static inline const float kWalkspeed = 0.00f; // 敵の移動速度

	static inline const float kWalkMotionAnglestart = -20.0f;

	static inline const float kWalkMotionAngleEnd = 30.0f;

	static inline const float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f; // 歩行モーションのタイマー

	// 死亡演出用に追加するメンバ変数
	Behavior behavior_ = Behavior::kRoot;           // 現在の状態
	bool isDead_ = false;                           // 完全に消滅したかどうかのフラグ
	bool isCollisionDisabled_ = false;
	float deadTimer_ = 0.0f;                        // 死亡アニメーション用タイマー
	static inline const float kDeadDuration = 1.0f; // 死亡演出の長さ（秒）

};