#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>

class Enemy {
public:
	// プレイヤーを参考に、モデル・カメラ・初期位置を受け取る初期化関数
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update();
	void Draw();

private:
	// 描画に必要なエンジン系のポインタ
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;

	// 位置・回転・スケールを管理するワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Vector3 velocity_ = {0, 0, 0}; // 敵の現在の速度

	static inline const float kWalkspeed = 0.05f; // 敵の移動速度

	static inline const float kWalkMotionAnglestart = -20.0f;

	static inline const float kWalkMotionAngleEnd = 30.0f;

	static inline const float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f; // 歩行モーションのタイマー
};