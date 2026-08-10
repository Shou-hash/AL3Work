#include "Enemy.h"
#include "BaseEnemyState.h"
#include "CollisionConfig.h" // ★ 衝突設定のインクルードを追加
#include "EnemyStateApproach.h"
#include "Player.h"
#include <cassert>

using namespace KamataEngine;

// デストラクタの実装
Enemy::~Enemy() {
	delete state_;
	ClearTimedCalls();
	for (EnemyBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

// Initialize 関数の実装（Playerを受け取るように変更）
void Enemy::Initialize(Model* model, uint32_t textureHandle, Player* player) {
	assert(model);
	assert(player);
	model_ = model;
	textureHandle_ = textureHandle;
	player_ = player;

	worldTransform_.Initialize();
	worldTransform_.translation_ = {20.0f, 0.0f, 30.0f};

	// ★ 当たり判定の半径を設定
	SetRadius(1.0f);

	// ★ 衝突属性とマスクを設定（自分は敵、相手は自分以外）
	SetCollisionAttribute(kCollisionAttributeEnemy);
	SetCollisionMask(~kCollisionAttributeEnemy);

	InitializeApproachPhase();
}

// GetWorldPosition を const 指定に合わせて修正
Vector3 Enemy::GetWorldPosition() const {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

// 接近フェーズの初期化処理
void Enemy::InitializeApproachPhase() {
	// 最初のステート（接近ステート）を設定
	ChangeState(new EnemyStateApproach(this));

	// 発射タイマーのセットアップ
	FireAndReset();
}

// 弾を発射してタイマーを再セットする処理
void Enemy::FireAndReset() {
	// 定期的に弾を発射するイベントをタイマー登録
	timedCalls_.push_back(new TimedCall(
	    [this]() {
		    Fire();
		    FireAndReset(); // 次の発射タイマーを再帰登録
	    },
	    kFireInterval));
}

// Vector3の演算用ヘルパー関数
inline Vector3 Normalize(const Vector3& v) {
	float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len != 0.0f) {
		return {v.x / len, v.y / len, v.z / len};
	}
	return {0.0f, 0.0f, 0.0f};
}

// 自機狙い弾の発射処理
void Enemy::Fire() {
	assert(model_);
	assert(player_);

	const float kBulletSpeed = 0.5f;

	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 enemyPos = GetWorldPosition();

	Vector3 diff = {playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z};
	Vector3 dir = Normalize(diff);

	Vector3 velocity = {dir.x * kBulletSpeed, dir.y * kBulletSpeed, dir.z * kBulletSpeed};

	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->Initialize(model_, enemyPos, velocity);

	// ★ ここで弾に Player を渡す
	newBullet->SetPlayer(player_);

	// ★ 敵の弾にも衝突属性とマスクを設定（敵陣営として扱う）
	newBullet->SetCollisionAttribute(kCollisionAttributeEnemy);
	newBullet->SetCollisionMask(~kCollisionAttributeEnemy);

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

void Enemy::OnCollision() {
	// 当たっても何もしない（資料の仕様通り）
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