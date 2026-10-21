#define NOMINMAX
#include "FinalBoss.h"
#include "MapChipField.h" // ★ 追加: マップチップ判定用
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
	delete spriteTargetMarker_; // ★ 予告マーカースプライト解放
	delete spriteHpBG_;
	delete spriteHpBar_;
}

void FinalBoss::Initialize(
    KamataEngine::Model* modelBody, const std::array<KamataEngine::Model*, kNumHands>& modelHands, const std::array<KamataEngine::Model*, kNumCrystals>& modelCrystals,
    KamataEngine::Model* modelCrystal22, // ★ 引数変更 (modelCrystal22)
    KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	modelBody_ = modelBody;
	modelHands_ = modelHands;
	modelCrystals_ = modelCrystals;   // ★ 代入
	modelCrystal22_ = modelCrystal22; // ★ 攻撃専用クリスタル
	camera_ = camera;

	// 基準位置の設定 (マップ上の初期位置からのオフセット調整)
	basePosition_ = position;

	// 背景の奥行きとしてZ軸のみ固定したい場合は下記のように設定します
	basePosition_.z = 6.5f;

	// ★ 描画オフセットX：ラスボスの手前（基準Xの 10 手前）で画面の右寄りに見えるよう、基準Xと視差係数から自動で決める
	drawOffset_.x = -(1.0f - parallaxFactor_) * basePosition_.x - 0.5f;

	// ルート変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = basePosition_;
	worldTransform_.rotation_.y = std::numbers::pi_v<float>;

	// スケールを小さく調整
	worldTransform_.scale_ = {0.0f, 0.0f, 0.0f}; // ★ 最初はサイズ0

	// ★ テクスチャの読み込みとパーティクル用スプライトの生成
	whiteTextureHandle_ = KamataEngine::TextureManager::Load("./Resources/white1x1.png");
	spriteParticle_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});

	// ★ 落下目標予告マーカー用スプライト生成
	spriteTargetMarker_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
	spriteHpBG_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {500.0f, 28.0f}, {0.08f, 0.08f, 0.10f, 0.95f}, {0.0f, 0.0f});
	spriteHpBar_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {500.0f, 28.0f}, {1.0f, 0.12f, 0.18f, 1.0f}, {0.0f, 0.0f});

	// ★ パーティクル描画用 WorldTransform の初期化
	particleWorldTransform_.Initialize();

	// 本体ローカルの初期化
	worldTransformBody_.Initialize();
	worldTransformCrystal22_.Initialize();

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

	// 22個のクリスタルの初期ローカルオフセットを設定（本体を中心にした扇状・不規則配置）
	crystalLocalOffsets_ = {
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
	    KamataEngine::Vector3{0.0f, 0.0f, 0.0f},
        KamataEngine::Vector3{0.0f, 0.0f, 0.0f}
    };

	for (size_t i = 0; i < kNumCrystals; ++i) {
		worldTransformCrystals_[i].Initialize();
		worldTransformCrystals_[i].translation_ = crystalLocalOffsets_[i];
	}

	// ★ 流星群用ワールドトランスフォーム初期化
	for (size_t i = 0; i < kNumMeteorShower; ++i) {
		meteorCrystals_[i].worldTransform.Initialize();
	}

	animTimer_ = 0.0f;
	crystalHidden_.fill(false);
	crystal22Landed_ = false;
	crystal22Flying_ = false;
	isFinalBossDefeated_ = false;
	finalBossHitEvent_ = false;
	crystal22HitCount_ = 0;
	finalBossHp_ = maxFinalBossHp_;
	crystal22Timer_ = 0.0f;
	hitReactionTimer_ = 0.0f;
	hitReactionRotation_ = 0.0f;
	isDead_ = false;
	isCollisionDisabled_ = true; // 奥行きにいるため直接の衝突判定は通常無効

	// ★ 接近前は待機状態にする（StartSpawnはまだ呼ばない）
	state_ = State::kStandby;
	frenzyWaitTimer_ = 0.0f;
	attackTimer_ = 0.0f;
	defeatTimer_ = 0.0f;
	isDefeatAnimationFinished_ = false;
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

// ★ 流星群攻撃の開始関数
void FinalBoss::StartAttack() {
	state_ = State::kAttack;
	attackTimer_ = 0.0f; // ★ 攻撃開始アニメーションのタイマーをリセット
	currentAttackIndex_ = 0;
	spawnIntervalTimer_ = 0.0f;
	crystal22Landed_ = false;
	crystal22Flying_ = false;
	crystal22HitCount_ = 0;
	crystal22HitsRequired_ = 1 + (rand() % 3);
	crystal22Timer_ = 0.0f;
	crystal22LandedTimer_ = 0.0f;
	meteorShowerStarted_ = false;                                  // ★ 追加: 流星群フラグ初期化
	attackDamageDone_ = false;                                     // ★ 追加: ダメージ済みフラグ初期化
	worldTransformCrystal22_.translation_ = {0.0f, -100.0f, 0.0f}; // 落下前は画面外に退避

	// 1〜21番目のクリスタルを不規則な順番で空へ発射する準備
	attackOrder_.clear();
	for (size_t i = 0; i < kMeteorCrystals; ++i) {
		attackOrder_.push_back(i);
		crystalHidden_[i] = false;
		crystalAttackInfos_[i].status = CrystalAttackStatus::kIdle;
		crystalAttackInfos_[i].timer = 0.0f;
	}
	for (size_t i = attackOrder_.size(); i > 1; --i) {
		size_t j = static_cast<size_t>(rand()) % i;
		std::swap(attackOrder_[i - 1], attackOrder_[j]);
	}
	crystalAttackInfos_[kMeteorCrystals].status = CrystalAttackStatus::kIdle;
	crystalAttackInfos_[kMeteorCrystals].timer = 0.0f;

	// 流星群パラメータ設定（※周りのクリスタルが飛ぶまで active = false にしておく）
	for (size_t i = 0; i < kNumMeteorShower; ++i) {
		auto& meteor = meteorCrystals_[i];
		meteor.active = false; // ★ 最初は非アクティブに設定
		meteor.timer = 0.0f;
		meteor.bounceCount = 0; // ★ 追加: バウンド回数リセット
		meteor.maxBounces = 3;  // ★ 追加: 3回バウンドして消滅

		// ★ 変更: 密度を減らすため発生遅延（delay）の間隔を広げる（パラパラと降るように調整）
		meteor.delay = static_cast<float>(i) * 0.28f + (static_cast<float>(rand() % 100) / 500.0f);

		float scaleVal = 0.25f + (static_cast<float>(rand() % 100) / 300.0f);
		meteor.scale = {scaleVal, scaleVal, scaleVal};

		float spawnX = basePosition_.x + (static_cast<float>(rand() % 300) / 10.0f - 15.0f);
		if (player_) {
			spawnX = player_->GetWorldTransform().translation_.x + (static_cast<float>(rand() % 400) / 10.0f - 20.0f);
		}

		// ★ 変更: 物理計算用の初期座標および速度設定（落下速度を緩やかに）
		meteor.position = {spawnX, basePosition_.y + 22.0f + (static_cast<float>(rand() % 100) / 10.0f), basePosition_.z};
		float targetX = spawnX + (static_cast<float>(rand() % 100) / 20.0f - 2.5f);
		meteor.velocity = {(targetX - spawnX) * 0.02f, -0.10f, 0.0f}; // ゆっくり落下開始

		// ★ 追加: 流星群が着地する地表のY座標を算出（落下予測マーカー用）
		float targetY = basePosition_.y - 3.0f;
		if (mapChipField_) {
			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition({spawnX, player_ ? player_->GetWorldTransform().translation_.y + 5.0f : basePosition_.y, 0.0f});
			for (int32_t y = 0; y < static_cast<int32_t>(mapChipField_->GetNumBlockVertical()); ++y) {
				MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.x, y);
				if ((type == MapChipType::kBlock && mapChipField_->GetBlockFaceByIndex(index.x, y) == BlockFace::kAbove) || type == MapChipType::kBlockFall) {
					KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, y);
					targetY = blockPos.y + 0.5f;
					break;
				}
			}
		}
		meteor.targetPosition = {targetX, targetY, basePosition_.z}; // ★ 正確な着地予定位置を設定

		meteor.worldTransform.translation_ = meteor.position;
		meteor.worldTransform.scale_ = meteor.scale;
	}
}

bool FinalBoss::HitCrystal22() {
	if (!crystal22Landed_ || crystal22Flying_ || isFinalBossDefeated_) {
		return false;
	}
	++crystal22HitCount_;
	if (crystal22HitCount_ >= crystal22HitsRequired_) {
		crystal22Flying_ = true;
		crystal22Landed_ = false;
		crystal22Timer_ = 0.0f;
		crystal22StartPosition_ = worldTransformCrystal22_.translation_;
		crystal22TargetPosition_ = {worldTransformBody_.matWorld_.m[3][0], worldTransformBody_.matWorld_.m[3][1], worldTransformBody_.matWorld_.m[3][2]};
		return true;
	}
	return false;
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

// ★ 撃破アニメーション用パーティクルの発生（本体から外側へ飛び散る）
void FinalBoss::EmitDefeatParticles() {
	KamataEngine::Vector3 center = {worldTransformBody_.matWorld_.m[3][0], worldTransformBody_.matWorld_.m[3][1], worldTransformBody_.matWorld_.m[3][2]};

	for (int i = 0; i < 6; ++i) {
		SpawnParticle p;

		float angle = static_cast<float>(rand() % 360) * (std::numbers::pi_v<float> / 180.0f);
		float speed = 0.05f + static_cast<float>(rand() % 150) / 1000.0f;

		p.position = {center.x + std::cos(angle) * 0.5f, center.y + std::sin(angle) * 0.5f, center.z};
		p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed, 0.0f};

		float scaleVal = 0.10f + static_cast<float>(rand() % 100) / 800.0f;
		p.scale = {scaleVal, scaleVal, scaleVal};
		p.color = {1.0f, 0.85f, 0.5f, 1.0f};
		p.maxLife = 1.0f;
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

namespace {
// スムーズステップ（edge0 以下で 0.0、edge1 以上で 1.0 になめらかに変化する）
float SmoothStep(float edge0, float edge1, float x) {
	float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
} // namespace

void FinalBoss::Update() {
	if (isDead_ || state_ == State::kStandby) {
		return;
	}

	animTimer_ += 1.0f / 60.0f;
	if (state_ == State::kAttack) {
		attackTimer_ += 1.0f / 60.0f; // ★ 攻撃開始からの経過時間
	}

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
	// 発狂後の待機状態 (kFrenzy) ＆ 攻撃状態 (kAttack) ロジック
	// =========================================================
	if (state_ == State::kFrenzy) {
		frenzyWaitTimer_ += 1.0f / 60.0f;
		// ★ ラストの星でダメージを与えてから待機状態（kFrenzy）で6秒以上経ったら流星群攻撃を再実行
		if (frenzyWaitTimer_ >= frenzyWaitDuration_ && !isFinalBossDefeated_) {
			StartAttack();
		}
	} else if (state_ == State::kAttack) {
		const float dt = 1.0f / 60.0f;
		spawnIntervalTimer_ += dt;

		// -------------------------------------------------------------
		// ① 最初の21個（kMeteorCrystals個）をランダムな順番で空へ飛び立たせる
		// -------------------------------------------------------------
		if (currentAttackIndex_ < attackOrder_.size() && spawnIntervalTimer_ >= spawnInterval_) {
			spawnIntervalTimer_ = 0.0f;
			size_t idx = attackOrder_[currentAttackIndex_++];
			CrystalAttackInfo& info = crystalAttackInfos_[idx];
			info.status = CrystalAttackStatus::kLaunching;
			info.timer = 0.0f;
			info.startPosition = {worldTransformCrystals_[idx].matWorld_.m[3][0], worldTransformCrystals_[idx].matWorld_.m[3][1], worldTransformCrystals_[idx].matWorld_.m[3][2]};
			info.currentWorldPos = info.startPosition;
		}

		// 飛び立ち中の上昇移動と消去処理
		for (size_t i = 0; i < kMeteorCrystals; ++i) {
			CrystalAttackInfo& info = crystalAttackInfos_[i];
			if (info.status == CrystalAttackStatus::kLaunching) {
				info.timer += dt;
				info.currentWorldPos.y += 0.55f;
				if (info.timer >= 0.55f) {
					info.status = CrystalAttackStatus::kIdle;
					crystalHidden_[i] = true; // 空へ飛び立ち完了して非表示化
				}
			}
		}

		// ★ 21個すべてのクリスタルが順番に発射され、かつ全員が完全に消えた（crystalHidden_ == true）か判定
		bool allLaunched = (currentAttackIndex_ >= attackOrder_.size());
		if (allLaunched) {
			for (size_t i = 0; i < kMeteorCrystals; ++i) {
				if (!crystalHidden_[i]) {
					allLaunched = false;
					break;
				}
			}
		}

		// -------------------------------------------------------------
		// ② 周りの21個がすべて飛び立ち終わったら、流星群の落下を開始する
		// -------------------------------------------------------------
		if (allLaunched && !meteorShowerStarted_) {
			meteorShowerStarted_ = true;
			for (size_t i = 0; i < kNumMeteorShower; ++i) {
				meteorCrystals_[i].active = true; // ★ 流星群を一斉アクティブ化
			}
		}

		bool allMeteorsFinished = meteorShowerStarted_;
		if (meteorShowerStarted_) {
			for (size_t i = 0; i < kNumMeteorShower; ++i) {
				auto& meteor = meteorCrystals_[i];
				if (!meteor.active) {
					continue;
				}

				allMeteorsFinished = false; // まだ落下中の流星が存在する

				if (meteor.delay > 0.0f) {
					meteor.delay -= dt;
					continue;
				}

				meteor.timer += dt;

				// ★ 変更: 落ちる速度を減らす（重力加速度を小さめに設定してゆっくり落下）
				const float gravity = 0.007f;
				meteor.velocity.y -= gravity;

				// ★ 変更: 座標の更新
				meteor.position.x += meteor.velocity.x;
				meteor.position.y += meteor.velocity.y;

				meteor.worldTransform.translation_ = meteor.position;
				meteor.worldTransform.rotation_.z += 0.12f;

				meteor.worldTransform.scale_ = meteor.scale;
				meteor.worldTransform.matWorld_ = MakeAffineMatrix(meteor.worldTransform.scale_, meteor.worldTransform.rotation_, meteor.worldTransform.translation_);
				meteor.worldTransform.TransferMatrix();

				// ★ 追加: プレイヤーとの当たり判定およびダメージ処理
				if (meteor.bounceCount == 0 && player_ && !player_->IsDead()) {
					Player::AABB playerAABB = player_->GetAABB();
					float marginX = 0.8f; // 当たり判定のマージン
					float marginY = 0.8f;

					if (meteor.position.x >= playerAABB.min.x - marginX && meteor.position.x <= playerAABB.max.x + marginX && meteor.position.y >= playerAABB.min.y - marginY &&
					    meteor.position.y <= playerAABB.max.y + marginY) {

						player_->OnCollision(); // ★ プレイヤーにダメージを与える（HPが減少しノックバック/死亡演出へ）
						meteor.active = false;  // ヒットした流星群は消滅
						continue;
					}
				}

				// ★ 変更: 地形との衝突判定（弾力性あるボールのような段階的バウンド処理）
				if (mapChipField_) {
					MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(meteor.position);
					MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.x, index.y);
					bool isGroundBlock = (type == MapChipType::kBlock && mapChipField_->GetBlockFaceByIndex(index.x, index.y) == BlockFace::kAbove);
					bool isFloatingTile = (type == MapChipType::kBlockFall);

					if (isGroundBlock || isFloatingTile) {
						// 落下中に着地した場合
						if (meteor.velocity.y < 0.0f) {
							if (meteor.bounceCount < meteor.maxBounces) {
								// ★ 跳ね返り（Y速度反転＆減衰により小さめのジャンプを繰り返す）
								meteor.velocity.y = -meteor.velocity.y * 0.5f;
								meteor.velocity.x *= 0.7f; // 横移動速度も少し減衰
								KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);
								meteor.position.y = blockPos.y + 0.5f; // 地面上に位置合わせ
								meteor.bounceCount++;
							} else {
								// ★ 規定回数（3回）バウンドしたら消滅
								meteor.active = false;
								continue;
							}
						}
					}
				}

				// ★ 追加: 画面外（下方）へ落ちすぎた場合の消去
				if (meteor.position.y < basePosition_.y - 12.0f) {
					meteor.active = false;
				}
			}
		}

		// -------------------------------------------------------------
		// ③ 流星群が全て降り終わったら、専用の finalBossCrystal_22 を落として地面に留まらせる
		// -------------------------------------------------------------
		if (allLaunched && allMeteorsFinished && !crystal22Landed_ && !crystal22Flying_ && !attackDamageDone_) {
			if (crystal22Timer_ == 0.0f) {
				KamataEngine::Vector3 playerPos = player_ ? player_->GetWorldTransform().translation_ : basePosition_;
				float targetX = playerPos.x + (static_cast<float>(rand() % 50) / 10.0f - 2.5f);
				float targetY = playerPos.y;

				if (mapChipField_) {
					MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition({targetX, playerPos.y + 5.0f, 0.0f});
					for (int32_t y = 0; y < static_cast<int32_t>(mapChipField_->GetNumBlockVertical()); ++y) {
						MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.x, y);
						if ((type == MapChipType::kBlock && mapChipField_->GetBlockFaceByIndex(index.x, y) == BlockFace::kAbove) || type == MapChipType::kBlockFall) {
							KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, y);
							targetY = blockPos.y + 0.5f;
							break;
						}
					}
				}

				crystal22StartPosition_ = {targetX, worldTransform_.translation_.y + 18.0f, 0.0f};
				crystal22TargetPosition_ = {targetX, targetY, 0.0f};
				worldTransformCrystal22_.translation_ = crystal22StartPosition_;
			}

			crystal22Timer_ += dt;

			float t = std::min(crystal22Timer_ / 1.5f, 1.0f);
			float easeT = t * t;
			worldTransformCrystal22_.translation_ = {
			    crystal22StartPosition_.x + (crystal22TargetPosition_.x - crystal22StartPosition_.x) * easeT,
			    crystal22StartPosition_.y + (crystal22TargetPosition_.y - crystal22StartPosition_.y) * easeT, 0.0f};

			if (mapChipField_) {
				MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(worldTransformCrystal22_.translation_);
				MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.x, index.y);
				bool isGround = (type == MapChipType::kBlock && mapChipField_->GetBlockFaceByIndex(index.x, index.y) == BlockFace::kAbove) || (type == MapChipType::kBlockFall);
				if (isGround) {
					KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);
					worldTransformCrystal22_.translation_.y = blockPos.y + 0.5f; // 地面上にジャストで着地
					crystal22Landed_ = true;
					crystal22Timer_ = 0.0f;
					crystal22LandedTimer_ = 0.0f;
				}
			}

			if (t >= 1.0f && !crystal22Landed_) {
				crystal22Landed_ = true; // ★ 地面に到着してそのまま固定・待機
				crystal22Timer_ = 0.0f;
				crystal22LandedTimer_ = 0.0f;
			}
		}

		// -------------------------------------------------------------
		// ④ 着地した finalBossCrystal_22 は、プレイヤーに打たれるまでステージにそのまま残る（タイムアウトなし）
		// -------------------------------------------------------------

		// -------------------------------------------------------------
		// ⑤ プレイヤーに打たれた専用クリスタルがラスボスへ飛んでダメージを与える
		// -------------------------------------------------------------
		if (crystal22Flying_) {
			crystal22Timer_ += dt;
			float t = std::min(crystal22Timer_ / 0.55f, 1.0f);
			float easeT = t * t;
			worldTransformCrystal22_.translation_ = {
			    crystal22StartPosition_.x + (crystal22TargetPosition_.x - crystal22StartPosition_.x) * easeT,
			    crystal22StartPosition_.y + (crystal22TargetPosition_.y - crystal22StartPosition_.y) * easeT + std::sin(t * std::numbers::pi_v<float>) * 2.0f,
			    crystal22StartPosition_.z + (crystal22TargetPosition_.z - crystal22StartPosition_.z) * easeT};

			if (t >= 1.0f) {
				crystal22Flying_ = false;
				finalBossHitEvent_ = true;
				attackDamageDone_ = true; // ★ 追加: ダメージを与えた（ここから全クリスタルの復帰を待って攻撃終了）
				finalBossHp_ = std::max(0, finalBossHp_ - 1);
				hitReactionTimer_ = 0.0f;
				hitReactionRotation_ = 0.35f;
				crystal22HitCount_ = 0;
				crystal22Timer_ = 0.0f;
				worldTransformCrystal22_.translation_ = {0.0f, -100.0f, 0.0f}; // ヒット後は画面外退避

				for (size_t i = 0; i < kMeteorCrystals; ++i) {
					crystalHidden_[i] = false;
					crystalAttackInfos_[i].status = CrystalAttackStatus::kSkyFalling;
					crystalAttackInfos_[i].timer = 0.0f;
					crystalAttackInfos_[i].startPosition = {
					    worldTransform_.translation_.x + (static_cast<float>(rand() % 200) / 10.0f - 10.0f), worldTransform_.translation_.y + 22.0f + static_cast<float>(rand() % 100) / 10.0f,
					    basePosition_.z};
					crystalAttackInfos_[i].targetPosition = {worldTransform_.translation_.x + crystalLocalOffsets_[i].x, worldTransform_.translation_.y + crystalLocalOffsets_[i].y, basePosition_.z};
					crystalAttackInfos_[i].currentWorldPos = crystalAttackInfos_[i].startPosition;
				}
				if (finalBossHp_ <= 0) {
					// ★ 撃破: 流星群とクリスタルの攻撃を全て止めて、撃破アニメーション（kDefeat）へ移行する
					isFinalBossDefeated_ = true;
					state_ = State::kDefeat;
					defeatTimer_ = 0.0f;
					for (auto& meteor : meteorCrystals_) {
						meteor.active = false;
					}
					for (size_t i = 0; i < kMeteorCrystals; ++i) {
						crystalHidden_[i] = false;
						crystalAttackInfos_[i].status = CrystalAttackStatus::kIdle;
						crystalAttackInfos_[i].timer = 0.0f;
					}
				}
			}
		}

		// 空から落ちた21個のクリスタルを元の位置へ戻す
		bool allReturned = true;
		for (size_t i = 0; i < kMeteorCrystals; ++i) {
			CrystalAttackInfo& info = crystalAttackInfos_[i];
			if (info.status != CrystalAttackStatus::kSkyFalling && info.status != CrystalAttackStatus::kReturning)
				continue;
			info.timer += dt;
			if (info.status == CrystalAttackStatus::kSkyFalling) {
				float t = std::min(info.timer / 0.8f, 1.0f);
				float easeT = t * t;
				info.currentWorldPos = {
				    info.startPosition.x + (info.targetPosition.x - info.startPosition.x) * easeT, info.startPosition.y + (info.targetPosition.y - info.startPosition.y) * easeT,
				    info.startPosition.z + (info.targetPosition.z - info.startPosition.z) * easeT};
				if (t >= 1.0f) {
					info.status = CrystalAttackStatus::kReturning;
					info.timer = 0.0f;
				}
				allReturned = false;
			} else if (info.timer >= 0.1f) {
				info.status = CrystalAttackStatus::kIdle;
				info.timer = 0.0f;
			} else {
				allReturned = false;
			}
		}

		// ★ ダメージを受けてクリスタルが全復帰したら、待機状態（kFrenzy）へ移行（ここから6秒タイマー開始）
		// ★ 修正: ダメージを与える前に終了判定が成立すると、打ち上げ途中で攻撃が打ち切られ、空のクリスタルが固まったまま残るため attackDamageDone_ を必須にする
		if (attackDamageDone_ && allReturned && !crystal22Landed_ && !crystal22Flying_ && currentAttackIndex_ >= attackOrder_.size()) {
			state_ = isFinalBossDefeated_ ? State::kDefeat : State::kFrenzy; // ★ 撃破していたら撃破アニメーションを続ける
			frenzyWaitTimer_ = 0.0f;
			crystal22Timer_ = 0.0f;
		}
	}

	// =========================================================
	// 撃破アニメーション (kDefeat) ロジック
	//   0.0 ~ 0.3: 痙攣（大きく震えて膨らむ） / 0.25 ~ 0.85: 崩れ落ちる / 0.8 ~ 1.0: 縮小して消える
	//   ※ 終了したら isDefeatAnimationFinished_ が true になり、GameScene 側でゴール追従カメラ演出が始まる
	// =========================================================
	if (state_ == State::kDefeat) {
		defeatTimer_ += 1.0f / 60.0f;
		const float defeatProgress = std::min(defeatTimer_ / kDefeatDuration, 1.0f);

		// 痙攣中と消える直前はパーティクルが飛び散る
		if (defeatProgress < 0.3f) {
			if (rand() % 3 == 0) {
				EmitDefeatParticles();
			}
		} else if (defeatProgress >= 0.8f) {
			EmitDefeatParticles();
		}

		// 最後に縮小して消える
		const float shrink = SmoothStep(0.8f, 1.0f, defeatProgress);
		const float scaleRate = 1.0f - shrink;
		worldTransform_.scale_ = {targetScale_.x * scaleRate, targetScale_.y * scaleRate, targetScale_.z * scaleRate};

		if (defeatTimer_ >= kDefeatDuration) {
			worldTransform_.scale_ = {0.0f, 0.0f, 0.0f};
			isDefeatAnimationFinished_ = true;
			state_ = State::kNormal;
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
	worldTransformBody_.scale_ = {1.0f, 1.0f, 1.0f};

	if (state_ == State::kFrenzyInit) {
		// 発狂の初期演出：本体が激しく高速シェイク
		float shakeAmount = 0.25f;
		worldTransformBody_.translation_.x = std::sin(frenzyTimer_ * 50.0f) * shakeAmount;
		worldTransformBody_.translation_.y = std::cos(frenzyTimer_ * 40.0f) * shakeAmount;
		worldTransformBody_.rotation_.z = std::sin(frenzyTimer_ * 35.0f) * 0.1f;
	}

	// ラスボス被撃時に Z 回転を加え、時間とともに元の角度へ戻す
	if (hitReactionTimer_ < 0.45f && hitReactionRotation_ != 0.0f) {
		hitReactionTimer_ += 1.0f / 60.0f;
		worldTransformBody_.rotation_.z = hitReactionRotation_ * (1.0f - std::min(hitReactionTimer_ / 0.45f, 1.0f));
	} else {
		worldTransformBody_.rotation_.z = 0.0f;
		hitReactionRotation_ = 0.0f;
	}

	// ★ 攻撃開始（溜め ➔ 咆哮）＆ 攻撃中のラスボス本体アニメーション
	if (state_ == State::kAttack) {
		const float introT = std::clamp(attackTimer_ / kAttackIntroDuration, 0.0f, 1.0f);
		float charge = 0.0f; // 溜め（息を吸い込んで膨らみ、背をそらす）
		float roar = 0.0f;   // 咆哮（一気に前へ振り下ろして元に戻る）
		if (introT < 0.7f) {
			charge = std::sin((introT / 0.7f) * std::numbers::pi_v<float> * 0.5f);
		} else {
			const float k = (introT - 0.7f) / 0.3f;
			charge = 1.0f - k * k;
			roar = std::sin(k * std::numbers::pi_v<float>);
		}

		float bodyScale = 1.0f + 0.20f * charge + 0.12f * roar;
		worldTransformBody_.translation_.y += 1.0f * charge;
		worldTransformBody_.rotation_.z += -0.20f * charge + 0.25f * roar;
		worldTransformBody_.rotation_.x += -0.15f * charge + 0.20f * roar;

		// 攻撃中: 怒りで脈動しながら左右にゆらぐ
		if (attackTimer_ >= kAttackIntroDuration) {
			bodyScale *= 1.0f + 0.04f * std::sin(animTimer_ * 3.0f);
			worldTransformBody_.rotation_.z += 0.07f * std::sin(animTimer_ * 1.6f);
			worldTransformBody_.translation_.y += 0.25f * std::sin(animTimer_ * 2.2f);
		}
		worldTransformBody_.scale_ = {bodyScale, bodyScale, bodyScale};
	}

	// ★ 撃破アニメーション: 痙攣 ➔ 傾いて崩れ落ちる
	if (state_ == State::kDefeat) {
		const float defeatProgress = std::min(defeatTimer_ / kDefeatDuration, 1.0f);
		const float convulsion = 1.0f - SmoothStep(0.0f, 0.5f, defeatProgress);
		const float fall = SmoothStep(0.25f, 0.85f, defeatProgress);

		worldTransformBody_.translation_.x += std::sin(defeatTimer_ * 55.0f) * 0.45f * convulsion;
		worldTransformBody_.translation_.y += std::cos(defeatTimer_ * 47.0f) * 0.35f * convulsion - 3.0f * fall * fall;
		worldTransformBody_.rotation_.z += std::sin(defeatTimer_ * 38.0f) * 0.12f * convulsion + 0.9f * fall;

		// 痙攣の間は大きく膨らむ
		const float bulge = 1.0f + 0.15f * std::sin(std::min(defeatProgress / 0.3f, 1.0f) * std::numbers::pi_v<float>);
		worldTransformBody_.scale_ = {bulge, bulge, bulge};
	}

	KamataEngine::Matrix4x4 localMatBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	// 本体ワールド行列 = 本体のローカル行列 × ルートワールド行列
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatBody, worldTransform_.matWorld_);
	worldTransformBody_.TransferMatrix();

	// =========================================================
	// 3. 7本の手のうごめきアニメーション ＆ 親子関係行列合成
	// =========================================================
	float handSpeed = (state_ == State::kAttack) ? 6.0f : ((state_ == State::kFrenzy) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f));
	float handSpread = (state_ == State::kAttack) ? (std::min(attackTimer_ / kAttackIntroDuration, 1.0f) * 2.4f + std::sin(animTimer_ * 4.0f) * 0.2f)
	                                              : ((state_ == State::kFrenzyInit) ? (std::min(frenzyTimer_ / 2.0f, 1.0f) * 1.8f) : ((state_ == State::kFrenzy) ? 1.5f : 0.0f));
	// ★ 撃破アニメーション: 崩れ落ちるにつれて手の動きが弱まり、だらりと垂れ下がる
	if (state_ == State::kDefeat) {
		const float handFall = SmoothStep(0.25f, 0.85f, std::min(defeatTimer_ / kDefeatDuration, 1.0f));
		handSpread = 1.5f * (1.0f - handFall);
		handSpeed = 6.0f * (1.0f - handFall) + 0.8f;
	}

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

	// =========================================================
	// 4. 22個のクリスタルの不規則な上下移動アニメーション ＆ 親子関係行列合成
	// =========================================================
	float baseCrystalSpeed = (state_ == State::kAttack) ? 6.0f : ((state_ == State::kFrenzy) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f));
	float crystalAmp = (state_ == State::kAttack) ? 1.0f : ((state_ == State::kFrenzy) ? 1.2f : 0.6f);

	for (size_t i = 0; i < kNumCrystals; ++i) {
		float phaseOffset = static_cast<float>(i) * 0.7f;

		// 各クリスタルごとに異なる上下移動のスピード倍率を設定
		float speedMultiplier = 0.6f + static_cast<float>((i * 5 + 3) % 11) * 0.15f;
		float currentSpeed = baseCrystalSpeed * speedMultiplier;

		// 周期の異なるサイン波を組み合わせて不規則な上下移動（Y軸）を計算
		float verticalOffset = std::sin(animTimer_ * currentSpeed + phaseOffset) * crystalAmp + std::sin(animTimer_ * (currentSpeed * 0.63f) + phaseOffset * 1.5f) * (crystalAmp * 0.35f);

		// X/Z軸および回転のゆらぎは行わず、純粋に不規則な上下移動（Y軸）のみを適用
		worldTransformCrystals_[i].translation_.x = crystalLocalOffsets_[i].x;
		worldTransformCrystals_[i].translation_.y = crystalLocalOffsets_[i].y + verticalOffset;
		worldTransformCrystals_[i].translation_.z = crystalLocalOffsets_[i].z;

		worldTransformCrystals_[i].rotation_ = {0.0f, 0.0f, 0.0f};

		// ★ 撃破アニメーション: クリスタルが本体から外側へ飛び散りながら落ちていく
		if (state_ == State::kDefeat) {
			const float scatter = SmoothStep(0.2f, 0.9f, std::min(defeatTimer_ / kDefeatDuration, 1.0f));
			const float dirX = crystalLocalOffsets_[i].x;
			const float dirY = crystalLocalOffsets_[i].y;
			const float len = std::sqrt(dirX * dirX + dirY * dirY) + 0.001f;
			worldTransformCrystals_[i].translation_.x += dirX / len * scatter * 8.0f;
			worldTransformCrystals_[i].translation_.y += dirY / len * scatter * 4.0f - scatter * scatter * 10.0f;
			worldTransformCrystals_[i].rotation_.z = scatter * (static_cast<float>(i % 5) - 2.0f) * 3.0f;
		}

		KamataEngine::Matrix4x4 localMatCrystal = MakeAffineMatrix(worldTransformCrystals_[i].scale_, worldTransformCrystals_[i].rotation_, worldTransformCrystals_[i].translation_);
		worldTransformCrystals_[i].matWorld_ = MultiplyMatrix(localMatCrystal, worldTransformBody_.matWorld_);
		if (i < kMeteorCrystals && (crystalAttackInfos_[i].status == CrystalAttackStatus::kLaunching || crystalAttackInfos_[i].status == CrystalAttackStatus::kSkyFalling ||
		                            crystalAttackInfos_[i].status == CrystalAttackStatus::kReturning)) {
			worldTransformCrystals_[i].matWorld_.m[3][0] = crystalAttackInfos_[i].currentWorldPos.x;
			worldTransformCrystals_[i].matWorld_.m[3][1] = crystalAttackInfos_[i].currentWorldPos.y;
			worldTransformCrystals_[i].matWorld_.m[3][2] = crystalAttackInfos_[i].currentWorldPos.z;
		}
		worldTransformCrystals_[i].TransferMatrix();
	}

	// ★ 攻撃専用クリスタルのワールド行列更新
	worldTransformCrystal22_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransformCrystal22_.rotation_ = {0.0f, 0.0f, animTimer_ * 2.0f};
	worldTransformCrystal22_.matWorld_ = MakeAffineMatrix(worldTransformCrystal22_.scale_, worldTransformCrystal22_.rotation_, worldTransformCrystal22_.translation_);
	worldTransformCrystal22_.TransferMatrix();
}

// ★ 現在降下・発射中の流星群（finalBossCrystal_22）のワールド座標リストを取得
std::vector<KamataEngine::Vector3> FinalBoss::GetActiveMeteorPositions() const {
	std::vector<KamataEngine::Vector3> positions;
	if (state_ != State::kAttack) {
		return positions;
	}

	// 流星群（meteorCrystals_）で現在アクティブかつ降下中の位置を追加
	for (size_t i = 0; i < kNumMeteorShower; ++i) {
		if (meteorCrystals_[i].active && meteorCrystals_[i].delay <= 0.0f) {
			positions.push_back(meteorCrystals_[i].worldTransform.translation_);
		}
	}

	// 専用クリスタルが落下中・着地中であればその位置も追加
	if (!crystal22Flying_ && (crystal22Landed_ || crystal22Timer_ > 0.0f)) {
		positions.push_back(worldTransformCrystal22_.translation_);
	}

	return positions;
}

// ★ 落下目標予告マーカーの描画（Sprite::PreDraw に Sprite::BlendMode::kAdd を渡して加算ブレンディング切り替え）
void FinalBoss::DrawTargetMarkers() {
	if (!spriteTargetMarker_ || !camera_ || state_ != State::kAttack) {
		return;
	}

	// ★ 画像仕様に従い、コマンドリストとブレンドモードを指定して加算ブレンディングを設定
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();
	if (dxCommon) {
		KamataEngine::Sprite::PreDraw(dxCommon->GetCommandList(), KamataEngine::Sprite::BlendMode::kAdd);
	} else {
		KamataEngine::Sprite::PreDraw();
	}

	// ★ 追加: 落下中の流星群の着地予定地点へ予告マーカーを描画
	if (meteorShowerStarted_) {
		for (size_t i = 0; i < kNumMeteorShower; ++i) {
			const auto& meteor = meteorCrystals_[i];
			// まだバウンドしていない（落下中かつアクティブな）流星群の予測地点に描画
			if (meteor.active && meteor.bounceCount == 0 && meteor.delay <= 0.0f) {
				KamataEngine::Vector2 screenPos = WorldToScreen(meteor.targetPosition, *camera_);
				if (screenPos.x < -100.0f || screenPos.x > 1380.0f || screenPos.y < -100.0f || screenPos.y > 820.0f) {
					continue;
				}

				float pulse = (std::sin(animTimer_ * 20.0f + static_cast<float>(i)) * 0.5f + 0.5f);
				float alpha = 0.6f + pulse * 0.4f;

				spriteTargetMarker_->SetPosition(screenPos);
				spriteTargetMarker_->SetSize({60.0f + pulse * 12.0f, 20.0f + pulse * 6.0f});
				spriteTargetMarker_->SetColor({1.0f, 0.6f, 0.1f, alpha * 0.8f});
				spriteTargetMarker_->Draw();
			}
		}
	}

	for (size_t i = 0; i < kNumCrystals; ++i) {
		const auto& info = crystalAttackInfos_[i];
		if (info.status != CrystalAttackStatus::kTargeting) {
			continue;
		}

		KamataEngine::Vector2 screenPos = WorldToScreen(info.targetPosition, *camera_);
		if (screenPos.x < -100.0f || screenPos.x > 1380.0f || screenPos.y < -100.0f || screenPos.y > 820.0f) {
			continue;
		}

		float pulse = (std::sin(info.timer * 25.0f) * 0.5f + 0.5f);
		float alpha = 0.5f + pulse * 0.5f;

		// ★ 最後の「finalBossCrystal_22」（インデックス kMeteorCrystals）はより大きく赤く光る強い予告マーカー
		if (i == kMeteorCrystals) {
			spriteTargetMarker_->SetPosition(screenPos);
			spriteTargetMarker_->SetSize({120.0f + pulse * 20.0f, 40.0f + pulse * 10.0f});
			spriteTargetMarker_->SetColor({1.0f, 0.1f, 0.1f, alpha});
		} else {
			spriteTargetMarker_->SetPosition(screenPos);
			spriteTargetMarker_->SetSize({80.0f + pulse * 15.0f, 24.0f + pulse * 8.0f});
			spriteTargetMarker_->SetColor({1.0f, 0.5f, 0.2f, alpha * 0.8f});
		}

		spriteTargetMarker_->Draw();
	}

	KamataEngine::Sprite::PostDraw();
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

		for (size_t i = 0; i < kMeteorCrystals; ++i) {
			if (modelCrystals_[i] && !crystalHidden_[i]) {
				modelCrystals_[i]->Draw(worldTransformCrystals_[i], *camera_);
			}
		}

		// ★ finalBossCrystal_22 を使用した流星群（不規則なスケールで多数描画）
		if (modelCrystal22_ && state_ == State::kAttack) {
			for (size_t i = 0; i < kNumMeteorShower; ++i) {
				if (meteorCrystals_[i].active && meteorCrystals_[i].delay <= 0.0f) {
					modelCrystal22_->Draw(meteorCrystals_[i].worldTransform, *camera_);
				}
			}
		}

		// ★ 最後にステージに落ちる finalBossCrystal_22（着地後はステージ上にそのまま残る）
		if (modelCrystal22_ && (crystal22Landed_ || crystal22Flying_ || worldTransformCrystal22_.translation_.y > -50.0f)) {
			modelCrystal22_->Draw(worldTransformCrystal22_, *camera_);
		}
	}

	// ★ 落下目標予告マーカーの描画（加算合成）
	DrawTargetMarkers();

	// ★ ラスボスが出現中（待機中でなく撃破されていない状態）なら常にHPバーを表示する
	if (state_ != State::kStandby && !isFinalBossDefeated_ && finalBossHp_ > 0 && spriteHpBG_ && spriteHpBar_) {
		const float barWidth = 280.0f;
		const float barHeight = 18.0f;
		const float hpRate = static_cast<float>(finalBossHp_) / static_cast<float>(maxFinalBossHp_);
		KamataEngine::Sprite::PreDraw();
		spriteHpBG_->SetPosition({500.0f, 28.0f});
		spriteHpBG_->SetSize({barWidth, barHeight});
		spriteHpBG_->Draw();
		spriteHpBar_->SetPosition({500.0f, 28.0f});
		spriteHpBar_->SetSize({barWidth * hpRate, barHeight});
		spriteHpBar_->Draw();
		KamataEngine::Sprite::PostDraw();
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
