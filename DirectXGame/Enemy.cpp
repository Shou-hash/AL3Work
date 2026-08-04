#include "Enemy.h"
#include "BaseEnemyState.h"
#include "EnemyStateApproach.h"
#include <cassert>

using namespace KamataEngine;

// デストラクタ
Enemy::~Enemy() {
	delete state_;

	for (EnemyBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();

	for (TimedCall* timedCall : timedCalls_) {
		delete timedCall;
	}
	timedCalls_.clear();
}

void Enemy::Initialize(Model* model, uint32_t textureHandle) {
	assert(model);
	// 1. 先にモデルとテクスチャを設定する！
	model_ = model;
	textureHandle_ = textureHandle;

	worldTransform_.Initialize();
	worldTransform_.translation_ = {0.0f, 2.0f, 40.0f};

	// 2. 初期ステートを設定
	ChangeState(new EnemyStateApproach(this));

	// 3. モデル等の準備が終わった後に接近フェーズの弾発射タイマーを1回だけセットする
	InitializeApproachPhase();
}

// 接近フェーズ初期化処理
void Enemy::InitializeApproachPhase() {
	// 以前のタイマーが残っていたらクリアしておく
	ClearTimedCalls();

	// 最初の発射イベントを実行＆予約
	FireAndReset();
}

// 弾を発射し、次の発射を予約する関数
void Enemy::FireAndReset() {
	// 1. 弾を発射
	Fire();

	// 2. 次の発射タイマーをセット
	std::function<void(void)> callback = std::bind(&Enemy::FireAndReset, this);
	TimedCall* timedCall = new TimedCall(callback, kFireInterval);
	timedCalls_.push_back(timedCall);
}

// 弾発射の実体関数
void Enemy::Fire() {
	assert(model_);

	// 手前へ飛ぶ速度ベクトル
	const float kBulletSpeed = -1.0f;
	Vector3 velocity(0.0f, 0.0f, kBulletSpeed);

	// 弾の生成と初期化
	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->Initialize(model_, worldTransform_.translation_, velocity);

	// 弾リストに登録
	bullets_.push_back(newBullet);
}

// 時限発動イベントのクリア
void Enemy::ClearTimedCalls() {
	for (TimedCall* timedCall : timedCalls_) {
		delete timedCall;
	}
	timedCalls_.clear();
}

void Enemy::Update() {
	// 完了した TimedCall を削除
	timedCalls_.remove_if([](TimedCall* timedCall) {
		if (timedCall->IsFinished()) {
			delete timedCall;
			return true;
		}
		return false;
	});

	// 時限発動イベントの更新
	for (TimedCall* timedCall : timedCalls_) {
		timedCall->Update();
	}

	// デスフラグの立った弾を削除
	bullets_.remove_if([](EnemyBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	// ステートの更新
	if (state_ != nullptr) {
		state_->Update();
	}

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

	// 弾の更新
	for (EnemyBullet* bullet : bullets_) {
		bullet->Update();
	}
}

void Enemy::Draw(const Camera& camera) {
	model_->Draw(worldTransform_, camera, textureHandle_);

	for (EnemyBullet* bullet : bullets_) {
		bullet->Draw(camera);
	}
}

void Enemy::ChangeState(BaseEnemyState* newState) {
	delete state_;
	state_ = newState;
}