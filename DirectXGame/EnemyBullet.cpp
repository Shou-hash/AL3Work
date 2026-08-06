#include "EnemyBullet.h"
#include "Player.h"  // Playerのクラス定義が必要
#include <algorithm> // std::clamp 用
#include <cassert>
#include <cmath>

using namespace KamataEngine;

// 内積の計算
inline float Dot(const Vector3& v1, const Vector3& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

// ベクトルの長さ（ノルム）
inline float Length(const Vector3& v) { return std::sqrt(Dot(v, v)); }

// ベクトルの正規化
inline Vector3 Normalize(const Vector3& v) {
	float len = Length(v);
	if (len != 0.0f) {
		return {v.x / len, v.y / len, v.z / len};
	}
	return {0.0f, 0.0f, 0.0f};
}

// Vector3 への加算演算子 (+=)
inline Vector3& operator+=(Vector3& lhs, const Vector3& rhs) {
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

// 球面線形補間 (Slerp) 関数の実装（ゼロ除算対策済み）
Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t) {
	float dot = Dot(v1, v2);

	// 誤差対策（-1.0f ～ 1.0f の範囲に収める）
	dot = std::clamp(dot, -1.0f, 1.0f);

	// 2つのベクトルがほぼ同じ向きの場合は、誤差やゼロ除算を避けるために通常のLerp（またはそのまま）を返す
	if (dot > 0.9995f) {
		Vector3 result = {v1.x + (v2.x - v1.x) * t, v1.y + (v2.y - v1.y) * t, v1.z + (v2.z - v1.z) * t};
		return Normalize(result);
	}

	// なす角 θ（シータ）を求める
	float theta = std::acos(dot);
	float sinTheta = std::sin(theta);

	// sin(θ) が 0 に極めて近い場合の安全策
	if (std::abs(sinTheta) < 0.0001f) {
		return v1;
	}

	// Slerp の公式
	float scale1 = std::sin((1.0f - t) * theta) / sinTheta;
	float scale2 = std::sin(t * theta) / sinTheta;

	return {scale1 * v1.x + scale2 * v2.x, scale1 * v1.y + scale2 * v2.y, scale1 * v1.z + scale2 * v2.z};
}

void EnemyBullet::Initialize(Model* model, const Vector3& position, const Vector3& velocity) {
	assert(model);
	model_ = model;

	textureHandle_ = TextureManager::Load("white1x1.png");

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	velocity_ = velocity;

	// 見た目を長細く設定
	worldTransform_.scale_.x = 0.5f;
	worldTransform_.scale_.y = 0.5f;
	worldTransform_.scale_.z = 3.0f;
}

void EnemyBullet::Update() {
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	// ホーミング処理
	if (player_ != nullptr) {
		// 1. 弾からプレイヤーへのベクトルを計算
		Vector3 playerPos = player_->GetWorldPosition();
		Vector3 toPlayer = {playerPos.x - worldTransform_.translation_.x, playerPos.y - worldTransform_.translation_.y, playerPos.z - worldTransform_.translation_.z};

		// 2. 現在の速さを記録しておく
		float bulletSpeed = Length(velocity_);

		if (bulletSpeed > 0.0001f) {
			// 3. ベクトルの正規化
			Vector3 dirToPlayer = Normalize(toPlayer);
			Vector3 currentDir = Normalize(velocity_);

			// 4. Slerp による旋回（t の値で曲がりやすさを調整 0.05f など）
			const float kHomingRate = 0.05f;
			Vector3 newDir = Slerp(currentDir, dirToPlayer, kHomingRate);

			// 5. 新しい速度を設定（方向 * 元の速さ）
			velocity_ = {newDir.x * bulletSpeed, newDir.y * bulletSpeed, newDir.z * bulletSpeed};
		}
	}

	// 座標移動
	worldTransform_.translation_ += velocity_;

	// 進行方向に合わせた見た目の回転制御
	worldTransform_.rotation_.y = std::atan2(velocity_.x, velocity_.z);
	float velocityXZ = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
	worldTransform_.rotation_.x = std::atan2(-velocity_.y, velocityXZ);

	// ワールド行列の再計算
	float sx = worldTransform_.scale_.x;
	float sy = worldTransform_.scale_.y;
	float sz = worldTransform_.scale_.z;

	float rx = worldTransform_.rotation_.x;
	float ry = worldTransform_.rotation_.y;

	float cx = std::cos(rx);
	float sx_rad = std::sin(rx);
	float cy = std::cos(ry);
	float sy_rad = std::sin(ry);

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

void EnemyBullet::OnCollision() {
	// 当たったら消える（デスフラグを立てる）
	isDead_ = true;
}

KamataEngine::Vector3 EnemyBullet::GetWorldPosition() const 
{ 
	return {worldTransform_.matWorld_.m[3][0], 
		worldTransform_.matWorld_.m[3][1],
		worldTransform_.matWorld_.m[3][2]}; 
}

void EnemyBullet::Draw(const Camera& camera) 
{
	model_->Draw(worldTransform_, camera, textureHandle_); 
}