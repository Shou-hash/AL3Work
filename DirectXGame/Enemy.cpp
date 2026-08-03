#include "Enemy.h"
#include "BaseEnemyState.h"
#include "EnemyStateApproach.h"

using namespace KamataEngine;

void Enemy::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	model_ = model;
	textureHandle_ = textureHandle;

	worldTransform_.Initialize();
	worldTransform_.translation_ = {0.0f, 2.0f, 40.0f};

	// 初期ステート（接近状態）を生成・設定
	ChangeState(new EnemyStateApproach(this));
}

void Enemy::Update() {
	// 現在のステートの更新を実行
	if (state_ != nullptr) {
		state_->Update();
	}

	// 行列の再計算
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

void Enemy::ChangeState(BaseEnemyState* newState) {
	// 古いステートを削除して入れ替える（Engine::changeStateと同じ仕組み）
	delete state_;
	state_ = newState;
}

void Enemy::Draw(const KamataEngine::Camera& camera) { model_->Draw(worldTransform_, camera, textureHandle_); }

Enemy::~Enemy() { delete state_; }