#include "EnemyBullet.h"
#include <cassert>
#include <cmath> // std::atan2, std::sqrt 用

using namespace KamataEngine;

// Vector3 への加算演算子 (+=) のオーバーロード
inline Vector3& operator+=(Vector3& lhs, const Vector3& rhs) {
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

void EnemyBullet::Initialize(Model* model, const Vector3& position, const Vector3& velocity) {
	assert(model);
	model_ = model;

	// テクスチャ管理
	textureHandle_ = TextureManager::Load("white1x1.png");

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	// 引数で受け取った速度を代入
	velocity_ = velocity;

	// 1. Z方向に長い形状にする (スケール変更)
	worldTransform_.scale_.x = 0.5f;
	worldTransform_.scale_.y = 0.5f;
	worldTransform_.scale_.z = 3.0f;

	// 2. Y軸まわりの角度 (θy) の計算
	worldTransform_.rotation_.y = std::atan2(velocity_.x, velocity_.z);

	// 3. XZ平面上の速度ベクトルの長さ (底辺) を求める
	float velocityXZ = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);

	// 4. X軸まわりの角度 (θx) の計算
	worldTransform_.rotation_.x = std::atan2(-velocity_.y, velocityXZ);
}

void EnemyBullet::Update() {
	// 時間経過でデス（タイマーカウントダウン）
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	// 座標移動
	worldTransform_.translation_ += velocity_;

	// 5. 回転行列（Rx, Ry）と拡大縮小、平行移動を考慮してワールド行列を計算
	float sx = worldTransform_.scale_.x;
	float sy = worldTransform_.scale_.y;
	float sz = worldTransform_.scale_.z;

	float rx = worldTransform_.rotation_.x;
	float ry = worldTransform_.rotation_.y;

	float cx = std::cos(rx);
	float sx_rad = std::sin(rx);
	float cy = std::cos(ry);
	float sy_rad = std::sin(ry);

	// Yaw(Y回転) -> Pitch(X回転) を合成したワールド行列
	worldTransform_.matWorld_.m[0][0] = sx * cy;
	worldTransform_.matWorld_.m[0][1] = 0.0f;
	worldTransform_.matWorld_.m[0][2] = -sx * sy_rad;
	worldTransform_.matWorld_.m[0][3] = 0.0f;

	worldTransform_.matWorld_.m[1][0] = sy * (sx_rad * sy_rad);
	worldTransform_.matWorld_.m[1][1] = sy * cx;
	worldTransform_.matWorld_.m[1][2] = sy * (sx_rad * cy);
	worldTransform_.matWorld_.m[1][3] = 0.0f;

	worldTransform_.matWorld_.m[2][0] = sz * (cx * sy_rad);
	worldTransform_.matWorld_.m[2][1] = sz * (-sx_rad);
	worldTransform_.matWorld_.m[2][2] = sz * (cx * cy);
	worldTransform_.matWorld_.m[2][3] = 0.0f;

	worldTransform_.matWorld_.m[3][0] = worldTransform_.translation_.x;
	worldTransform_.matWorld_.m[3][1] = worldTransform_.translation_.y;
	worldTransform_.matWorld_.m[3][2] = worldTransform_.translation_.z;
	worldTransform_.matWorld_.m[3][3] = 1.0f;

	worldTransform_.TransferMatrix();
}

void EnemyBullet::Draw(const Camera& camera) { model_->Draw(worldTransform_, camera, textureHandle_); }