#pragma once
#include "Kamataengine.h"

class Player;

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

class CameraController {
public:
	// 初期化時にカメラのポインタを受け取るように変更
	void Initialize(KamataEngine::Camera* camera);

	void Update();
	void Reset();

	// 追従対象の設定
	void SetTarget(Player* target) { target_ = target; }

	void SetMovableArea(const Rect& area) { movableArea_ = area; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;
	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.5f, -15.0f};

	KamataEngine::Vector3 targetPosition_;

	static inline const float kInterpolationRate = 0.3f;

	static inline const float kVelocityBias = 5.0f;

	Rect movableArea_ = {5, 100, 0, 100};

	static inline const Rect margin_ = {-2.0f, 2.0f, -1.0f, 2.0f};
};