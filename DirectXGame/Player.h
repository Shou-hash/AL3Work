#pragma once
#include "Kamataengine.h"
#include <3d\WorldTransform.h>

// 左右の向きを表す列挙型
enum class LRDirection {
	kLeft,
	kRight,
};

class Player {
public:
	Player();
	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update();

	void Draw();

	// 必要に応じて速度やトランスフォームを取得できるゲッター
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	KamataEngine::Vector3 velocity_ = {};
	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.1f;
	static inline const float kLimitRunSpeed = 5.0f;
	static inline const float kTimeTurn = 0.5f;

	static inline const float kGravityAcceleration = 0.5f;
	static inline const float kLimitFallSpeed = 2.0f;
	static inline const float kJumpAcceleration = 2.0f;

private:

	bool onGround_ = true;

	float turnFirstRotationY_ = 0.0f;

	float turnTimer_ = 0.0f;

	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	LRDirection lrDirection_ = LRDirection::kRight;
};