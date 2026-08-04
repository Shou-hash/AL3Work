#include "Enemy.h"
#include "BaseEnemyState.h"
#include "EnemyStateApproach.h"
#include <cassert>

using namespace KamataEngine;

Enemy::~Enemy() {
	delete state_;

	// 弾のメモリ解放
	for (EnemyBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

void Enemy::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	assert(model);
	model_ = model;
	textureHandle_ = textureHandle;

	worldTransform_.Initialize();
	worldTransform_.translation_ = {0.0f, 2.0f, 40.0f};

	// 初期ステート（接近状態）を設定
	ChangeState(new EnemyStateApproach(this));

	// 接近フェーズ初期化呼び出し（出現と同時に即発射はしない）
	ApproachInitialize();
}

void Enemy::Update() {
	// デスフラグの立った弾を削除
	bullets_.remove_if([](EnemyBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	// --- 1. ステートの更新 ---
	if (state_ != nullptr) {
		state_->Update();
	}

	// ※もしステートクラス(EnemyStateApproach)から呼び出していない場合は、
	// 接近フェーズ中の処理としてここで ApproachUpdate() を呼び出します。
	ApproachUpdate();

	// --- 2. 敵本体の行列更新 ---
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

	// --- 3. 弾更新（すべての弾を更新） ---
	for (EnemyBullet* bullet : bullets_) {
		bullet->Update();
	}
}

void Enemy::ApproachInitialize() {
	// 発射タイマーを初期化
	fireTimer_ = kFireInterval;
}

void Enemy::ApproachUpdate() {
	// 発射タイマーカウントダウン
	--fireTimer_;

	// 指定時間に達した（タイマーが0以下になった）
	if (fireTimer_ <= 0) {
		// 弾を発射
		Fire();

		// 発射タイマーを初期化（リセット）
		fireTimer_ = kFireInterval;
	}
}

void Enemy::Fire() {
	assert(model_);

	// 弾の速度ベクトル（手前に飛ばす速度: -1.0f）
	const float kBulletSpeed = -1.0f;
	Vector3 velocity(0.0f, 0.0f, kBulletSpeed);

	// 弾を生成して初期化
	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->Initialize(model_, worldTransform_.translation_, velocity);

	// 弾を登録
	bullets_.push_back(newBullet);
}

void Enemy::Draw(const KamataEngine::Camera& camera) {
	// 敵本体の描画
	model_->Draw(worldTransform_, camera, textureHandle_);

	// 敵弾の描画
	for (EnemyBullet* bullet : bullets_) {
		bullet->Draw(camera);
	}
}

void Enemy::ChangeState(BaseEnemyState* newState) {
	delete state_;
	state_ = newState;
}