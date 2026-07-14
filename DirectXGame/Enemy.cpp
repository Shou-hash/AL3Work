#include "Enemy.h"
#include "Matrix4x4.h"
#define _USE_MATH_DEFINES // 数学定数を使うために定義
#include <algorithm>
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
	worldTransform_.rotation_ = {0.0f, -90.0f * (std::numbers::pi_v<float> / 180.0f), 0.0f}; // 初期回転をラジアンに変換

	velocity_ = {-kWalkspeed, 0, 0};

	walkTimer_ = 0.0f;

	// 状態リセット
	behavior_ = Behavior::kRoot;
	isDead_ = false;
	isCollisionDisabled_ = false;
	deadTimer_ = 0.0f;
}

void Enemy::OnCollision(const Player* player) { (void)player; }

// 死亡演出開始のトリガー
void Enemy::OnDead() {
	// すでに死亡状態なら何もしない
	if (behavior_ == Behavior::kDead) {
		return;
	}

	behavior_ = Behavior::kDead;
	isCollisionDisabled_ = true;
	deadTimer_ = 0.0f;
	velocity_ = {0.0f, 0.0f, 0.0f}; // 移動を停止
}

// 通常状態の更新
void Enemy::BehaviorRootUpdate() {
	// 既存の移動処理
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
	float degree = kWalkMotionAnglestart + (kWalkMotionAngleEnd - kWalkMotionAnglestart) * (param + 1.0f) / 2.0f;

	// 度数法（degree）からラジアン（radian）に変換して、Z軸の回転に代入する
	worldTransform_.rotation_.z = degree * (std::numbers::pi_v<float> / 270.0f);

	// 通常時は縮小しないようにスケールを 1.0f に維持
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
}

// 死亡状態のアニメーション更新 (追加)
void Enemy::BehaviorDeadUpdate() {
	deadTimer_ += 1.0f / 60.0f;

	// 進行度を 0.0f ~ 1.0f にクランプ
	float t = std::clamp(deadTimer_ / kDeadDuration, 0.0f, 1.0f);

	// アニメーションの挙動設定（例：上方向にふわっと浮き上がりながら回転）
	// サインカーブなどを使って放物線上に跳ねさせることも可能です
	worldTransform_.translation_.y += 0.05f; // 上に上昇
	worldTransform_.rotation_.y += 0.1f;     // スピンさせる

	// スケールを徐々に小さくする（1.0f から 0.0f へ）
	float scale = 1.0f - t;
	worldTransform_.scale_ = {scale, scale, scale};

	// タイマーが規定時間に達したら完全に死亡（消滅）とする
	if (t >= 1.0f) {
		isDead_ = true;
	}
}

void Enemy::Update() {

	// 状態に応じて先に座標・回転・スケールの更新処理を行う
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kDead:
		BehaviorDeadUpdate();
		break;
	}

	// 更新が終わった正しい値を用いて、アフィン変換行列を計算して matWorld_ に代入する
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 計算した行列を GPU（定数バッファ）に転送
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	// モデルとカメラが正しく設定されていれば描画する
	if (isDead_) {
		return;
	}

	if (modelEnemy_) {
		modelEnemy_->Draw(worldTransform_, *camera_);
	}
}

Enemy::AABB Enemy::GetAABB() const {
	// 既に死亡状態の場合は当たり判定を無くす（あるいは無効な値を返す）
	if (isCollisionDisabled_) {
		return AABB{
		    {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f}
        };
	}

	AABB aabb;
	const auto& pos = worldTransform_.translation_;
	aabb.min = {pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f};
	aabb.max = {pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f};
	return aabb;
}