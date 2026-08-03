#include "Enemy.h"

using namespace KamataEngine;

// Vector3 への加算演算子 (+=) のオーバーロード
inline Vector3& operator+=(Vector3& lhs, const Vector3& rhs) {
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

void Enemy::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	// 引数で受け取ったモデルとテクスチャハンドルを保持
	model_ = model;
	textureHandle_ = textureHandle;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// 初期座標の設定 (例: 画面奥の Y=2, Z=40 の位置)
	worldTransform_.translation_ = {0.0f, 2.0f, 40.0f};

	// 初期フェーズの設定
	phase_ = Phase::Approach;
}

void Enemy::Update() {
	// フェーズごとの処理
	switch (phase_) {
	case Phase::Approach:
	default:
		ApproachUpdate();
		break;
	case Phase::Leave:
		LeaveUpdate();
		break;
	}

	// 行列（matWorld_）の再計算
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

	// 行列の転送
	worldTransform_.TransferMatrix();
}

// 接近フェーズの更新
void Enemy::ApproachUpdate() {
	// 接近フェーズの速度（手前に進む）
	Vector3 approachVelocity = {0.0f, 0.0f, -0.2f};

	// 移動（ベクトルを加算）
	worldTransform_.translation_ += approachVelocity;

	// 規定の位置に到達したら離脱フェーズへ移行
	if (worldTransform_.translation_.z < 0.0f) {
		phase_ = Phase::Leave;
	}
}

// 離脱フェーズの更新
void Enemy::LeaveUpdate() {
	// 離脱フェーズの速度（斜め上奥に離脱）
	Vector3 leaveVelocity = {-0.1f, 0.1f, -0.2f};

	// 移動（ベクトルを加算）
	worldTransform_.translation_ += leaveVelocity;
}

void Enemy::Draw(const KamataEngine::Camera& camera) {
	// 3Dモデルの描画
	model_->Draw(worldTransform_, camera, textureHandle_);
}

Enemy::~Enemy() {}