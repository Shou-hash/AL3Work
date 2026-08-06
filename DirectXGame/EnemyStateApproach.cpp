#include "EnemyStateApproach.h"
#include "Enemy.h"
#include "EnemyStateLeave.h"
#include <cmath> // std::sin, std::cos を使用

// コンストラクタ
EnemyStateApproach::EnemyStateApproach(Enemy* enemy) : BaseEnemyState("State Approach", enemy) {}

void EnemyStateApproach::Update() {
	DebugLog(); // ログ表示

	if (enemy_ == nullptr) {
		return;
	}

	// 経過フレームを加算
	frameCount_++;

	// Enemy から WorldTransform を取得
	KamataEngine::WorldTransform& transform = enemy_->GetWorldTransform();

	// Z軸方向：手前へ進行（一定速度）
	const float kForwardSpeed = -0.1f;
	transform.translation_.z += kForwardSpeed;

	//// X軸方向：正弦波（Sin）で大きく左右に旋回
	//const float kWaveSpeedX = 0.05f; // 横揺れの速さ（周期）
	//const float kAmplitudeX = 0.25f; // 横揺れの振り幅
	//transform.translation_.x += std::sin(frameCount_ * kWaveSpeedX) * kAmplitudeX;

	//// Y軸方向：余弦波（Cos）で上下にも少し揺らして立体感を出す
	//const float kWaveSpeedY = 0.025f; // 縦揺れの速さ
	//const float kAmplitudeY = 0.08f;  // 縦揺れの振り幅
	//transform.translation_.y += std::cos(frameCount_ * kWaveSpeedY) * kAmplitudeY;

	// z < 0.0f に達したら「離脱ステート」へ遷移！
	if (transform.translation_.z < 0.0f) {
		enemy_->ChangeState(new EnemyStateLeave(enemy_));
	}
}