#define NOMINMAX
#include "FinalBoss.h"
#include "Matrix4x4.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

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

FinalBoss::~FinalBoss() {
	// ★ スプライトリソースの解放
	delete spriteParticle_;
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
	worldTransform_.rotation_.y = std::numbers::pi_v<float>;

	// スケールを小さく調整（例: 2.0f から 1.0f や 1.2f などお好みのサイズに変更）
	worldTransform_.scale_ = {0.0f, 0.0f, 0.0f}; // ★ 最初はサイズ0

	// ★ テクスチャの読み込みとパーティクル用スプライトの生成
	whiteTextureHandle_ = KamataEngine::TextureManager::Load("./Resources/white1x1.png");
	spriteParticle_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});

	// ★ パーティクル描画用 WorldTransform の初期化
	particleWorldTransform_.Initialize();

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

	// ★ 接近前は待機状態にする（StartSpawnはまだ呼ばない）
	state_ = State::kStandby;
	particles_.clear();
}

// ★ 出現アニメーションのトリガー関数
void FinalBoss::StartSpawn() {
	if (state_ == State::kStandby) {
		state_ = State::kSpawn;
		spawnTimer_ = 0.0f;
		particles_.clear();
		worldTransform_.scale_ = {0.0f, 0.0f, 0.0f};
	}
}

// ★ 発狂アニメーション開始関数
void FinalBoss::StartFrenzy() {
	if (state_ == State::kNormal || state_ == State::kSpawn) {
		state_ = State::kFrenzyInit;
		frenzyTimer_ = 0.0f;
	}
}

// ★ 3Dワールド座標から2Dスクリーン座標への変換関数
KamataEngine::Vector2 FinalBoss::WorldToScreen(const KamataEngine::Vector3& worldPos, const KamataEngine::Camera& camera) {
	KamataEngine::Matrix4x4 matVP = MultiplyMatrix(camera.matView, camera.matProjection);

	float x = worldPos.x * matVP.m[0][0] + worldPos.y * matVP.m[1][0] + worldPos.z * matVP.m[2][0] + matVP.m[3][0];
	float y = worldPos.x * matVP.m[0][1] + worldPos.y * matVP.m[1][1] + worldPos.z * matVP.m[2][1] + matVP.m[3][1];
	float w = worldPos.x * matVP.m[0][3] + worldPos.y * matVP.m[1][3] + worldPos.z * matVP.m[2][3] + matVP.m[3][3];

	if (w <= 0.0f) {
		return {-9999.0f, -9999.0f};
	}

	float ndcX = x / w;
	float ndcY = y / w;

	// スクリーンサイズ (1280x720) に変換
	float screenX = (ndcX + 1.0f) * 0.5f * 1280.0f;
	float screenY = (1.0f - ndcY) * 0.5f * 720.0f;

	return {screenX, screenY};
}

// ★ 出現パーティクルの発生関数
void FinalBoss::EmitSpawnParticles() {
	KamataEngine::Vector3 center = worldTransform_.translation_;

	for (int i = 0; i < 4; ++i) {
		SpawnParticle p;

		float angle = static_cast<float>(rand() % 360) * (std::numbers::pi_v<float> / 180.0f);
		float radius = 2.0f + static_cast<float>(rand() % 150) / 50.0f;

		p.position = {
		    center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius + (static_cast<float>(rand() % 100) / 100.0f - 0.5f),
		    center.z + (static_cast<float>(rand() % 100) / 100.0f - 0.5f)};

		p.velocity = {(center.x - p.position.x) * 0.04f, (center.y - p.position.y) * 0.04f, (center.z - p.position.z) * 0.04f};

		float scaleVal = 0.08f + static_cast<float>(rand() % 100) / 1000.0f;
		p.scale = {scaleVal, scaleVal, scaleVal};
		p.color = {1.0f, 1.0f, 1.0f, 1.0f};
		p.maxLife = 0.8f;
		p.currentLife = 0.0f;

		particles_.push_back(p);
	}
}

// ★ パーティクルの更新処理
void FinalBoss::UpdateParticles() {
	for (auto it = particles_.begin(); it != particles_.end();) {
		it->currentLife += 1.0f / 60.0f;
		if (it->currentLife >= it->maxLife) {
			it = particles_.erase(it);
		} else {
			it->position.x += it->velocity.x;
			it->position.y += it->velocity.y;
			it->position.z += it->velocity.z;

			++it;
		}
	}
}

void FinalBoss::Update() {
	if (isDead_ || state_ == State::kStandby) {
		return;
	}

	animTimer_ += 1.0f / 60.0f;

	// =========================================================
	// 出現アニメーション (kSpawn) ロジック
	// =========================================================
	if (state_ == State::kSpawn) {
		spawnTimer_ += 1.0f / 60.0f;
		float progress = std::min(spawnTimer_ / spawnDuration_, 1.0f);

		// パーティクル発生＆更新
		EmitSpawnParticles();

		// イージング（SmoothStep）でスケールを徐々に大きく拡大
		float easeProgress = progress * progress * (3.0f - 2.0f * progress);
		worldTransform_.scale_ = {targetScale_.x * easeProgress, targetScale_.y * easeProgress, targetScale_.z * easeProgress};

		// 登場時の微小な振動効果
		float shake = (1.0f - progress) * 0.15f;
		worldTransformBody_.translation_.x = std::sin(animTimer_ * 40.0f) * shake;
		worldTransformBody_.translation_.y = std::cos(animTimer_ * 35.0f) * shake;

		// ★ 規定時間（出現完了）に達したら直接「発狂初期演出（kFrenzyInit）」へ切替！
		if (spawnTimer_ >= spawnDuration_) {
			worldTransform_.scale_ = targetScale_;
			state_ = State::kFrenzyInit; // ★ 発狂初期演出に切り替え
			frenzyTimer_ = 0.0f;
		}
	}

	// パーティクルの移動更新
	UpdateParticles();

	// 発狂初期演出タイマー更新
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

// ★ パーティクルの描画処理（white1x1.png スプライトを使用した正確な描画）
void FinalBoss::DrawParticles() {
	if (!spriteParticle_ || particles_.empty() || !camera_) {
		return;
	}

	KamataEngine::Sprite::PreDraw();
	for (const auto& p : particles_) {
		KamataEngine::Vector2 screenPos = WorldToScreen(p.position, *camera_);
		if (screenPos.x < -100.0f || screenPos.x > 1380.0f || screenPos.y < -100.0f || screenPos.y > 820.0f) {
			continue;
		}

		spriteParticle_->SetPosition(screenPos);
		spriteParticle_->SetSize({p.scale.x * 200.0f, p.scale.y * 200.0f});
		spriteParticle_->SetColor(p.color);
		spriteParticle_->Draw();
	}
	KamataEngine::Sprite::PostDraw();
}

void FinalBoss::Draw() {
	if (isDead_ || !camera_ || state_ == State::kStandby) {
		return;
	}

	// ボス本体および手の描画（スケールが0より大きい場合のみ描画）
	if (worldTransform_.scale_.x > 0.001f) {
		if (modelBody_) {
			modelBody_->Draw(worldTransformBody_, *camera_);
		}

		for (size_t i = 0; i < kNumHands; ++i) {
			if (modelHands_[i]) {
				modelHands_[i]->Draw(worldTransformHands_[i], *camera_);
			}
		}
	}

	// ★ 出現パーティクルの描画（スプライト描画パイプラインで描画）
	DrawParticles();
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