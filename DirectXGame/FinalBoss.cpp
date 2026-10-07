#define NOMINMAX
#include "FinalBoss.h"
#include "Matrix4x4.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdlib>

KamataEngine::Matrix4x4 FinalBoss::MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b) {
	KamataEngine::Matrix4x4 r = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float sum = 0.0f;
			for (int k = 0; k < 4; ++k) {
				sum += a.m[i][k] * b.m[k][j];
			}
			r.m[i][j] = sum;
		}
	}
	return r;
}

void FinalBoss::Initialize(KamataEngine::Model* modelBody, const std::array<KamataEngine::Model*, kNumHands>& modelHands, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {

	modelBody_ = modelBody;
	modelHands_ = modelHands;
	camera_ = camera;

	// 基準位置の設定 (マップ上の初期位置からのオフセット調整)
	basePosition_ = position;

	// 背景の奥行きとしてZ軸のみ固定したい場合は下記のように設定します
	basePosition_.z = 6.5f;

	// ルート変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = basePosition_;

	// Y軸回転を90度（π/2ラジアン）回転させて正面を向かせる
	// （モデルの向きに応じて反転が必要な場合は -std::numbers::pi_v<float> / 2.0f にしてください）
	worldTransform_.rotation_.y = std::numbers::pi_v<float>;

	// スケールを小さく調整（例: 2.0f から 1.0f や 1.2f などお好みのサイズに変更）
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};

	// 本体ローカルの初期化
	worldTransformBody_.Initialize();

	// 7本の手の初期ローカルオフセットを設定（本体を中心にした扇状・不規則配置）
	handLocalOffsets_ = {
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f}
	};

	for (size_t i = 0; i < kNumHands; ++i) {
		worldTransformHands_[i].Initialize();
		worldTransformHands_[i].translation_ = handLocalOffsets_[i];
	}

	animTimer_ = 0.0f;
	isDead_ = false;
	isCollisionDisabled_ = true; // 奥行きにいるため直接の衝突判定は通常無効
}

// ★ 追加: 発狂状態への移行トリガー
void FinalBoss::StartFrenzy() {
	if (state_ == State::kNormal) {
		state_ = State::kFrenzyInit;
		frenzyTimer_ = 0.0f;
	}
}

void FinalBoss::Update() {
	if (isDead_)
		return;

	animTimer_ += 1.0f / 60.0f;

	// 発狂初期演出タイマー更新（2秒後に発狂本番へ移行）
	if (state_ == State::kFrenzyInit) {
		frenzyTimer_ += 1.0f / 60.0f;
		if (frenzyTimer_ >= 2.0f) {
			state_ = State::kFrenzy;
		}
	}

	// =========================================================
	// 1. 上下ゆらゆら移動 ＆ 視差（パララックス）計算
	// =========================================================
	
	// 発狂状態に応じてゆらゆら速度と振り幅を変更
	float floatSpeed = (state_ == State::kFrenzy) ? 3.0f : 1.2f;
	float floatAmp = (state_ == State::kFrenzy) ? 0.8f : 0.4f;
	float floatOffsetY = std::sin(animTimer_ * floatSpeed) * floatAmp;

	// ★ 基準座標 (basePosition_) + 描画オフセット (drawOffset_) に対して視差と揺らぎを適応
	if (camera_) {
		worldTransform_.translation_.x = (basePosition_.x + drawOffset_.x) + camera_->translation_.x * (1.0f - parallaxFactor_);
		worldTransform_.translation_.y = (basePosition_.y + drawOffset_.y) + camera_->translation_.y * (1.0f - parallaxFactor_) + floatOffsetY;
		worldTransform_.translation_.z = basePosition_.z + drawOffset_.z;
	}

	// ラスボス全体の不気味な浮遊・揺らぎ運動
	worldTransform_.translation_.y += std::sin(animTimer_ * 1.2f) * 0.4f;

	// ルート（親）のワールド行列を生成
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// =========================================================
	// 2. 本体のローカル行列計算と合成
	// =========================================================

	worldTransformBody_.translation_ = {0.0f, 0.0f, 0.0f};
	worldTransformBody_.rotation_ = {0.0f, 0.0f, 0.0f};

	if (state_ == State::kFrenzyInit) {
		// 発狂の初期演出：本体が激しく高速シェイク
		float shakeAmount = 0.25f;
		worldTransformBody_.translation_.x = std::sin(frenzyTimer_ * 50.0f) * shakeAmount;
		worldTransformBody_.translation_.y = std::cos(frenzyTimer_ * 40.0f) * shakeAmount;
		worldTransformBody_.rotation_.z = std::sin(frenzyTimer_ * 35.0f) * 0.1f;
	}

	KamataEngine::Matrix4x4 localMatBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	// 本体ワールド行列 = 本体のローカル行列 × ルートワールド行列
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatBody, worldTransform_.matWorld_);
	worldTransformBody_.TransferMatrix();

	// =========================================================
	// 3. 7本の手のうごめきアニメーション ＆ 親子関係行列合成
	// =========================================================
	float handSpeed = (state_ == State::kFrenzy) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f);
	float handSpread = (state_ == State::kFrenzyInit) ? (std::min(frenzyTimer_ / 2.0f, 1.0f) * 1.8f) : ((state_ == State::kFrenzy) ? 1.5f : 0.0f);

	for (size_t i = 0; i < kNumHands; ++i) {
		float phaseOffset = static_cast<float>(i) * 0.8f;

		// 扇状に手が外側へ広がるオフセット値
		float angle = (static_cast<float>(i) / static_cast<float>(kNumHands)) * std::numbers::pi_v<float> * 2.0f;
		float spreadX = std::cos(angle) * handSpread;
		float spreadY = std::sin(angle) * handSpread;

		worldTransformHands_[i].translation_.x = handLocalOffsets_[i].x + spreadX + std::cos(animTimer_ * handSpeed + phaseOffset) * 0.4f;
		worldTransformHands_[i].translation_.y = handLocalOffsets_[i].y + spreadY + std::sin(animTimer_ * (handSpeed * 1.2f) + phaseOffset) * 0.5f;

		worldTransformHands_[i].rotation_.z = std::sin(animTimer_ * handSpeed + phaseOffset) * 0.3f;
		worldTransformHands_[i].rotation_.x = std::cos(animTimer_ * handSpeed + phaseOffset) * 0.2f;

		KamataEngine::Matrix4x4 localMatHand = MakeAffineMatrix(worldTransformHands_[i].scale_, worldTransformHands_[i].rotation_, worldTransformHands_[i].translation_);
		worldTransformHands_[i].matWorld_ = MultiplyMatrix(localMatHand, worldTransformBody_.matWorld_);
		worldTransformHands_[i].TransferMatrix();
	}
}

void FinalBoss::Draw() {
	if (isDead_ || !camera_)
		return;

	// 背景の奥に描画するため、PreDraw〜PostDraw を適切に使用
	if (modelBody_) {
		modelBody_->Draw(worldTransformBody_, *camera_);
	}

	for (size_t i = 0; i < kNumHands; ++i) {
		if (modelHands_[i]) {
			modelHands_[i]->Draw(worldTransformHands_[i], *camera_);
		}
	}
}

void FinalBoss::OnCollision(Player* player) { (void)player; }

void FinalBoss::OnDead() { isDead_ = true; }

BaseEnemy::AABB FinalBoss::GetAABB() const {
	// 奥行きに配置されているため当たり判定は空を返す
	return AABB{
	    {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
}