#include "Enemy.h"
#include "Matrix4x4.h"
#define _USE_MATH_DEFINES // 数学定数を使うために定義
#include <cmath>
#include <numbers> // std::numbers::pi を使うために追加

void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	// 各種ポインタの受け取り
	modelEnemy_ = model;
	camera_ = camera;

	// ワールドトランスフォームの初期化と位置設定
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	// 必要に応じてスケールや回転の初期値を設定
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, -90.0f, 0.0f};

	velocity_ = {-kWalkspeed, 0, 0};

	walkTimer_ = 0.0f;
}

void Enemy::Update() {
	// 1. 移動処理
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	walkTimer_ += 1.0f / 60.0f;

	// サインカーブの計算（周期 kWalkMotionTime でループする -1.0f ~ +1.0f の値）
	float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);

	// 値の範囲を 0.0f ~ 1.0f に加工し、角度の線形補間（Lerp）を行う
	float degree = kWalkMotionAnglestart + (kWalkMotionAngleEnd - kWalkMotionAnglestart) * ((param + 1.0f) / 2.0f);

	// 度数法（degree）からラジアン（radian）に変換して、Z軸の回転に代入する
	worldTransform_.rotation_.z = degree * (std::numbers::pi_v<float> / 270.0f);
	
	// Player と同様に、アフィン変換行列を計算して matWorld_ に代入
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 計算した行列を GPU（定数バッファ）に転送
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	// モデルとカメラが正しく設定されていれば描画する
	if (modelEnemy_ && camera_) {
		KamataEngine::Model::PreDraw();
		modelEnemy_->Draw(worldTransform_, *camera_);
		KamataEngine::Model::PostDraw();
	}
}