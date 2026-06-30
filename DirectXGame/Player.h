#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <list>

class MapChipField;
class CameraController;
class Enemy;

enum class LRDirection {
	kLeft,
	kRight,
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

	Player();
	~Player();

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

	// 敵との衝突判定
	void CheckEnemyCollision(const std::list<Enemy*>& enemies);

	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.5f;
	static inline const float kLimitRunSpeed = 2.0f;
	static inline const float kTimeTurn = 0.8f;
	static inline const float kGravityAcceleration = 0.08f;
	static inline const float kLimitFallSpeed = 1.0f;
	static inline const float kJumpAcceleration = 1.0f;
	static inline const float kAttenuationLanding = 0.2f;
	static inline const float kGroundSearchOffset = 0.01f;

private:
	void Move();
	void MapCollision(CollisionMapInfo& info);
	void MapCollisionTop(CollisionMapInfo& info);
	void MapCollisionBottom(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);
	void ApplyGroundingStatus(const CollisionMapInfo& info);
	void CheckScreenEdgeCollision();
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

private:
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
};