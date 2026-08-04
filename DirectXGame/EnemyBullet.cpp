#include "EnemyBullet.h"
#include <cassert>

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
}

void EnemyBullet::Update() {
	// 時間経過でデス（タイマーカウントダウン）
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	// 座標移動
	worldTransform_.translation_ += velocity_;

	// 行列更新
	worldTransform_.matWorld_ = {
	    worldTransform_.scale_.x,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    worldTransform_.scale_.y,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    worldTransform_.scale_.z,
	    0.0f,
	    worldTransform_.translation_.x,
	    worldTransform_.translation_.y,
	    worldTransform_.translation_.z,
	    1.0f};

	worldTransform_.TransferMatrix();
}

void EnemyBullet::Draw(const Camera& camera) 
{
	model_->Draw(worldTransform_, camera, textureHandle_);
}