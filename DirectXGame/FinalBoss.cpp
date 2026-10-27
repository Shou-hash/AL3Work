#define NOMINMAX
#include "FinalBoss.h"
#include "AudioManager.h" // ★ 音声再生用
#include "MapChipField.h"
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

namespace {
float SmoothStep(float edge0, float edge1, float x) {
	float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

float LerpF(float a, float b, float t) { return a + (b - a) * t; }

// ★ 軽量化: 同時に存在できるパーティクルの最大数
constexpr size_t kMaxParticles = 500;

// ★ 3Dワールド座標から2Dスクリーン座標への変換
KamataEngine::Vector2 ProjectToScreen(const KamataEngine::Vector3& worldPos, const KamataEngine::Matrix4x4& matVP) {
	float x = worldPos.x * matVP.m[0][0] + worldPos.y * matVP.m[1][0] + worldPos.z * matVP.m[2][0] + matVP.m[3][0];
	float y = worldPos.x * matVP.m[0][1] + worldPos.y * matVP.m[1][1] + worldPos.z * matVP.m[2][1] + matVP.m[3][1];
	float w = worldPos.x * matVP.m[0][3] + worldPos.y * matVP.m[1][3] + worldPos.z * matVP.m[2][3] + matVP.m[3][3];

	if (w <= 0.0f) {
		return {-9999.0f, -9999.0f};
	}

	float ndcX = x / w;
	float ndcY = y / w;

	return {(ndcX + 1.0f) * 0.5f * 1280.0f, (1.0f - ndcY) * 0.5f * 720.0f};
}
} // namespace

FinalBoss::~FinalBoss() {
	delete spriteParticle_;
	delete spriteTargetMarker_;
	delete spriteHpBG_;
	delete spriteHpBar_;
}

void FinalBoss::Initialize(
    KamataEngine::Model* modelBody, const std::array<KamataEngine::Model*, kNumHands>& modelHands, const std::array<KamataEngine::Model*, kNumCrystals>& modelCrystals,
    KamataEngine::Model* modelCrystal22, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	modelBody_ = modelBody;
	modelHands_ = modelHands;
	modelCrystals_ = modelCrystals;
	modelCrystal22_ = modelCrystal22;
	camera_ = camera;

	basePosition_ = position;
	basePosition_.z = 6.5f;

	drawOffset_.x = -(1.0f - parallaxFactor_) * basePosition_.x - 0.5f;

	worldTransform_.Initialize();
	worldTransform_.translation_ = basePosition_;
	worldTransform_.rotation_.y = std::numbers::pi_v<float>;
	worldTransform_.scale_ = {0.0f, 0.0f, 0.0f};

	whiteTextureHandle_ = KamataEngine::TextureManager::Load("./Resources/white1x1.png");
	spriteParticle_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
	spriteTargetMarker_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
	spriteHpBG_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {500.0f, 28.0f}, {0.08f, 0.08f, 0.10f, 0.95f}, {0.0f, 0.0f});
	spriteHpBar_ = KamataEngine::Sprite::Create(whiteTextureHandle_, {500.0f, 28.0f}, {1.0f, 0.12f, 0.18f, 1.0f}, {0.0f, 0.0f});

	particleWorldTransform_.Initialize();
	worldTransformBody_.Initialize();
	worldTransformCrystal22_.Initialize();

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

	for (size_t i = 0; i < kNumMeteorShower; ++i) {
		meteorCrystals_[i].worldTransform.Initialize();
	}

	for (auto& pillar : doomPillars_) {
		pillar.status = DoomPillarStatus::kInactive;
		pillar.crystalTransform.Initialize();
	}

	animTimer_ = 0.0f;
	crystalHidden_.fill(false);
	crystal22Landed_ = false;
	crystal22Flying_ = false;
	isFinalBossDefeated_ = false;
	finalBossHitEvent_ = false;
	crystal22HitCount_ = 0;
	maxFinalBossHp_ = 6;
	finalBossHp_ = maxFinalBossHp_;
	crystal22Timer_ = 0.0f;
	hitReactionTimer_ = 0.0f;
	hitReactionRotation_ = 0.0f;
	isDead_ = false;
	isCollisionDisabled_ = true;

	pendingDown_ = false;
	downWarningTimer_ = 0.0f;
	downTimer_ = 0.0f;
	bodyHitConsumed_ = false;

	isAwakened_ = false;
	awakeningTimer_ = 0.0f;
	awakenRoared_ = false;
	awakenRoarGlow_ = 0.0f;
	awakenStartRotX_ = 0.0f;
	awakenRiseOffset_ = {0.0f, 0.0f, 0.0f};
	dodgingAttackTimer_ = 0.0f;
	doomCount_ = 0;
	doomPunch_ = 0.0f;
	doomStrikeIntensity_ = 0.0f;
	doomStrikeEvent_ = false;

	state_ = State::kStandby;
	frenzyWaitTimer_ = 0.0f;
	frenzyWaitDuration_ = 6.0f;
	attackTimer_ = 0.0f;
	defeatTimer_ = 0.0f;
	isDefeatAnimationFinished_ = false;
	isDefeatZoomFinished_ = false;
	particles_.clear();
}

void FinalBoss::StartSpawn() {
	if (state_ == State::kStandby) {
		state_ = State::kSpawn;
		spawnTimer_ = 0.0f;
		particles_.clear();
		worldTransform_.scale_ = {0.0f, 0.0f, 0.0f};
		AudioManager::GetInstance()->PlaySE(SEType::kBossSpawn); // ★ ラスボス出現SE再生
	}
}

void FinalBoss::StartFrenzy() {
	if (state_ == State::kNormal || state_ == State::kSpawn) {
		state_ = State::kFrenzyInit;
		frenzyTimer_ = 0.0f;
	}
}

// ★ 覚醒状態への切り替え処理
void FinalBoss::TriggerAwakening() {
	isAwakened_ = true;
	state_ = State::kAwakening;
	awakeningTimer_ = 0.0f;
	awakenRoared_ = false;
	awakenRoarGlow_ = 0.0f;
	dodgingAttackTimer_ = 0.0f;
	frenzyWaitDuration_ = 2.0f; // ★ 覚醒時の攻撃頻度を上げるためインターバルを大幅短縮（6.0s ➔ 2.0s）
	maxFinalBossHp_ = 10;       // ★ 覚醒後にHP増量（6 -> 10）
	finalBossHp_ = maxFinalBossHp_;
	pendingDown_ = false;
	bodyHitConsumed_ = false;
	attackDamageDone_ = false;
	crystal22Landed_ = false;
	crystal22Flying_ = false;
	worldTransformCrystal22_.translation_ = {0.0f, -100.0f, 0.0f};

	AudioManager::GetInstance()->PlaySE(SEType::kBossSkill); // ★ 追加: 覚醒開始SE再生

	for (auto& pillar : doomPillars_) {
		pillar.status = DoomPillarStatus::kInactive;
	}

	// ★ 伏せていた位置・傾きから起き上がるために現在のずれを記録
	awakenStartRotX_ = worldTransformBody_.rotation_.x;
	const KamataEngine::Vector3 anchored = GetAnchoredPosition();
	awakenRiseOffset_ = {worldTransform_.translation_.x - anchored.x, worldTransform_.translation_.y - anchored.y, worldTransform_.translation_.z - anchored.z};

	for (size_t i = 0; i < kMeteorCrystals; ++i) {
		crystalHidden_[i] = false;
		crystalAttackInfos_[i].status = CrystalAttackStatus::kIdle;
	}
}

// ★ カメラの視差に合わせた本体の定位置
KamataEngine::Vector3 FinalBoss::GetAnchoredPosition() const {
	KamataEngine::Vector3 pos = {basePosition_.x + drawOffset_.x, basePosition_.y + drawOffset_.y, basePosition_.z + drawOffset_.z};
	if (!camera_) {
		return pos;
	}

	const float floatSpeed = (state_ == State::kFrenzy || isAwakened_) ? 3.0f : 1.2f;
	const float floatAmp = (state_ == State::kFrenzy || isAwakened_) ? 0.8f : 0.4f;
	const float floatOffsetY = std::sin(animTimer_ * floatSpeed) * floatAmp;

	pos.x += camera_->translation_.x * (1.0f - parallaxFactor_);
	pos.y += camera_->translation_.y * (1.0f - parallaxFactor_) + floatOffsetY + std::sin(animTimer_ * 1.2f) * 0.4f;
	return pos;
}

void FinalBoss::StartAttack() {
	state_ = State::kAttack;
	attackTimer_ = 0.0f;
	currentAttackIndex_ = 0;
	spawnIntervalTimer_ = 0.0f;
	crystal22Landed_ = false;
	crystal22Flying_ = false;
	crystal22HitCount_ = 0;
	crystal22HitsRequired_ = 1 + (rand() % 3);
	crystal22Timer_ = 0.0f;
	crystal22LandedTimer_ = 0.0f;
	meteorShowerStarted_ = false;
	attackDamageDone_ = false;
	pendingDown_ = false;
	bodyHitConsumed_ = false;
	worldTransformCrystal22_.translation_ = {0.0f, -100.0f, 0.0f};

	AudioManager::GetInstance()->PlaySE(SEType::kBossSkill); // ★ ラスボススキル発動SE再生

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

	for (size_t i = 0; i < kNumMeteorShower; ++i) {
		auto& meteor = meteorCrystals_[i];
		meteor.active = false;
		meteor.timer = 0.0f;
		meteor.bounceCount = 0;
		meteor.maxBounces = 3;

		meteor.delay = static_cast<float>(i) * 0.28f + (static_cast<float>(rand() % 100) / 500.0f);

		float scaleVal = 0.25f + (static_cast<float>(rand() % 100) / 300.0f);
		meteor.scale = {scaleVal, scaleVal, scaleVal};

		float spawnX = basePosition_.x + (static_cast<float>(rand() % 300) / 10.0f - 15.0f);
		if (player_) {
			spawnX = player_->GetWorldTransform().translation_.x + (static_cast<float>(rand() % 400) / 10.0f - 20.0f);
		}

		meteor.position = {spawnX, basePosition_.y + 22.0f + (static_cast<float>(rand() % 100) / 10.0f), basePosition_.z};
		float targetX = spawnX + (static_cast<float>(rand() % 100) / 20.0f - 2.5f);
		meteor.velocity = {(targetX - spawnX) * 0.02f, -0.10f, 0.0f};

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
		meteor.targetPosition = {targetX, targetY, basePosition_.z};

		meteor.worldTransform.translation_ = meteor.position;
		meteor.worldTransform.scale_ = meteor.scale;
	}
}

bool FinalBoss::HitCrystal22() {
	if (!crystal22Landed_ || crystal22Flying_ || isFinalBossDefeated_) {
		return false;
	}
	++crystal22HitCount_;
	AudioManager::GetInstance()->PlaySE(SEType::kBossCrystalHit); // ★ ラスボスがクリスタルに攻撃されたときSE再生

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

KamataEngine::Vector2 FinalBoss::WorldToScreen(const KamataEngine::Vector3& worldPos, const KamataEngine::Camera& camera) {
	KamataEngine::Matrix4x4 matVP = MultiplyMatrix(camera.matView, camera.matProjection);
	return ProjectToScreen(worldPos, matVP);
}

void FinalBoss::EmitSpawnParticles() {
	if (particles_.size() >= kMaxParticles) {
		return;
	}
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
		p.color = isAwakened_ ? KamataEngine::Vector4{1.0f, 0.2f, 0.1f, 1.0f} : KamataEngine::Vector4{1.0f, 1.0f, 1.0f, 1.0f};
		p.maxLife = 0.8f;
		p.currentLife = 0.0f;

		particles_.push_back(p);
	}
}

void FinalBoss::EmitDefeatParticles() {
	if (particles_.size() >= kMaxParticles) {
		return;
	}
	KamataEngine::Vector3 center = {worldTransformBody_.matWorld_.m[3][0], worldTransformBody_.matWorld_.m[3][1], worldTransformBody_.matWorld_.m[3][2]};

	for (int i = 0; i < 6; ++i) {
		SpawnParticle p;

		float angle = static_cast<float>(rand() % 360) * (std::numbers::pi_v<float> / 180.0f);
		float speed = 0.05f + static_cast<float>(rand() % 150) / 1000.0f;

		p.position = {center.x + std::cos(angle) * 0.5f, center.y + std::sin(angle) * 0.5f, center.z};
		p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed, 0.0f};

		float scaleVal = 0.10f + static_cast<float>(rand() % 100) / 800.0f;
		p.scale = {scaleVal, scaleVal, scaleVal};
		p.color = isAwakened_ ? KamataEngine::Vector4{1.0f, 0.15f, 0.05f, 1.0f} : KamataEngine::Vector4{1.0f, 0.85f, 0.5f, 1.0f};
		p.maxLife = 1.0f;
		p.currentLife = 0.0f;

		particles_.push_back(p);
	}
}

void FinalBoss::UpdateParticles() {
	if (particles_.empty()) {
		return;
	}

	for (auto& p : particles_) {
		p.currentLife += 1.0f / 60.0f;
		p.position.x += p.velocity.x;
		p.position.y += p.velocity.y;
		p.position.z += p.velocity.z;
	}

	particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const SpawnParticle& p) { return p.currentLife >= p.maxLife; }), particles_.end());
}

// =========================================================
// ★ 覚醒専用攻撃「終末の裁き」
//   クリスタルが1個ずつ空から落下して空中静止 ➔ 先端から画像の射線照射 ➔ 復帰を繰り返す
// =========================================================
void FinalBoss::StartDodgingAttack() {
	state_ = State::kDodgingAttack;
	dodgingAttackTimer_ = 0.0f;
	doomCount_ = 0;
	doomPunch_ = 0.0f;
	doomStrikeIntensity_ = 0.0f;
	doomStrikeEvent_ = false;
	for (auto& pillar : doomPillars_) {
		pillar.status = DoomPillarStatus::kInactive;
	}

	AudioManager::GetInstance()->PlaySE(SEType::kBossSkill); // ★ ラスボススキル発動SE再生
}

float FinalBoss::FindGroundY(float x, float referenceY) const {
	const float fallbackY = referenceY - 1.0f;
	if (!mapChipField_) {
		return fallbackY;
	}

	MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition({x, referenceY, 0.0f});
	float bestTop = -1.0e9f;
	bool found = false;
	for (int32_t y = 0; y < static_cast<int32_t>(mapChipField_->GetNumBlockVertical()); ++y) {
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.x, y);
		const bool isGround = (type == MapChipType::kBlock && mapChipField_->GetBlockFaceByIndex(index.x, y) == BlockFace::kAbove) || type == MapChipType::kBlockFall;
		if (!isGround) {
			continue;
		}
		const float top = mapChipField_->GetMapChipPositionByIndex(index.x, y).y + 0.5f;
		if (top <= referenceY + 0.6f && top > bestTop) {
			bestTop = top;
			found = true;
		}
	}
	return found ? bestTop : fallbackY;
}

void FinalBoss::SpawnSingleDoomPillar() {
	DoomPillar* slot = nullptr;
	for (auto& pillar : doomPillars_) {
		if (pillar.status == DoomPillarStatus::kInactive) {
			slot = &pillar;
			break;
		}
	}
	if (!slot) {
		return;
	}

	AudioManager::GetInstance()->PlaySE(SEType::kBossSkill); // ★ 追加: 終末の裁きクリスタル降下開始SE再生

	float playerX = basePosition_.x;
	float playerY = basePosition_.y;
	if (player_) {
		playerX = player_->GetWorldTransform().translation_.x;
		playerY = player_->GetWorldTransform().translation_.y;
	}

	// ★ 1回目はプレイヤーの直上、以降はプレイヤー周辺のランダム位置にクリスタルを設置
	float offsetX = (doomCount_ == 0) ? 0.0f : ((static_cast<float>(rand() % 100) / 100.0f * 2.0f - 1.0f) * 4.0f);

	slot->status = DoomPillarStatus::kFalling;
	slot->x = playerX + offsetX;
	slot->groundY = FindGroundY(slot->x, playerY);
	// ★ 空中静止的高さを一定範囲（5.5f ~ 9.5f）の中でランダムに設定
	slot->targetY = slot->groundY + 5.5f + (static_cast<float>(rand() % 100) / 100.0f * 4.0f);
	// ★ 手前（プレイヤーに近いZ軸座標）に描画するためZ座標を調整
	slot->z = 0.0f;
	slot->timer = 0.0f;

	// ★ 攻撃回数が経過するごとにクリスタルの落下速度と射線照射までの速度をどんどん加速させる
	float speedFactor = 1.0f + static_cast<float>(doomCount_) * 0.35f;
	slot->fallDuration = (std::max)(0.10f, kDoomFallDuration / speedFactor);
	slot->telegraphDuration = (std::max)(0.18f, kDoomTelegraphDuration / speedFactor);

	slot->hitDone = false;
	slot->crystalY = slot->groundY + 18.0f; // ★ 空中上空の初期高度
	slot->crystalTransform.Initialize();
	slot->crystalTransform.translation_ = {slot->x, slot->crystalY, slot->z};
	slot->crystalTransform.rotation_ = {0.0f, 0.0f, 0.0f}; // ★ 回転させない
	slot->crystalTransform.scale_ = {1.2f, 1.2f, 1.2f};
}

void FinalBoss::EmitDoomSparks(float x, float groundY) {
	if (particles_.size() >= kMaxParticles) {
		return;
	}

	for (int i = 0; i < 6; ++i) {
		SpawnParticle p;
		p.position = {x + (static_cast<float>(rand() % 100) / 100.0f * 2.0f - 1.0f) * kDoomPillarHalfWidth, groundY + 0.1f, 0.0f};
		p.velocity = {(static_cast<float>(rand() % 100) / 100.0f * 2.0f - 1.0f) * 0.03f, 0.04f + static_cast<float>(rand() % 100) / 1000.0f, 0.0f};
		const float scaleVal = 0.05f + static_cast<float>(rand() % 100) / 2500.0f;
		p.scale = {scaleVal, scaleVal, scaleVal};
		p.color = {1.0f, 0.55f, 0.2f, 1.0f};
		p.maxLife = 0.7f;
		p.currentLife = 0.0f;
		particles_.push_back(p);
	}
}

void FinalBoss::UpdateDodgingAttack(float dt) {
	dodgingAttackTimer_ += dt;
	doomPunch_ = (std::max)(0.0f, doomPunch_ - dt * 3.0f);

	// ★ 1個ずつ順番にクリスタルを降下・出現させる制御
	if (dodgingAttackTimer_ >= kDoomChargeDuration && doomCount_ < kDoomTotalCount) {
		bool hasActivePillar = false;
		for (const auto& pillar : doomPillars_) {
			if (pillar.status == DoomPillarStatus::kFalling || pillar.status == DoomPillarStatus::kTelegraph || pillar.status == DoomPillarStatus::kStrike) {
				hasActivePillar = true;
				break;
			}
		}

		if (!hasActivePillar) {
			SpawnSingleDoomPillar();
			++doomCount_;
		}
	}

	if (dodgingAttackTimer_ < kDoomChargeDuration && (rand() % 3) == 0) {
		EmitSpawnParticles();
	}

	const float playerX = player_ ? player_->GetWorldTransform().translation_.x : basePosition_.x;

	float targetIntensity = 0.0f;
	bool anyAlive = false;
	bool struckNow = false;
	float nearestStrikeDist = 1.0e9f;
	float nearestLightDist = 1.0e9f;

	const float pillarHalfWidth = kDoomPillarHalfWidth;

	for (auto& pillar : doomPillars_) {
		if (pillar.status == DoomPillarStatus::kInactive) {
			continue;
		}

		anyAlive = true;
		pillar.timer += dt;
		const float distToPlayer = std::fabs(pillar.x - playerX);

		// ★ 1. 空から降下して空中静止位置 (targetY) へ到達
		if (pillar.status == DoomPillarStatus::kFalling) {
			const float startY = pillar.groundY + 18.0f;
			const float targetY = pillar.targetY;

			float t = std::clamp(pillar.timer / pillar.fallDuration, 0.0f, 1.0f);
			float easeT = t * t * (3.0f - 2.0f * t);
			pillar.crystalY = startY + (targetY - startY) * easeT;

			pillar.crystalTransform.translation_ = {pillar.x, pillar.crystalY, pillar.z};
			pillar.crystalTransform.rotation_ = {0.0f, 0.0f, 0.0f}; // ★ 回転させない
			pillar.crystalTransform.scale_ = {1.2f, 1.2f, 1.2f};
			pillar.crystalTransform.matWorld_ = MakeAffineMatrix(pillar.crystalTransform.scale_, pillar.crystalTransform.rotation_, pillar.crystalTransform.translation_);
			pillar.crystalTransform.TransferMatrix();

			if (pillar.timer >= pillar.fallDuration) {
				pillar.status = DoomPillarStatus::kTelegraph;
				pillar.timer = 0.0f;
				AudioManager::GetInstance()->PlaySE(SEType::kBossSkillActive); // ★ 追加: 射線照射開始（予告）SE再生
			}
		}
		// ★ 2. 空中でピタッと静止し、下の先端から地面へ画像の射線（照射ビーム）を伸ばす
		else if (pillar.status == DoomPillarStatus::kTelegraph) {
			pillar.crystalY = pillar.targetY; // 静止

			pillar.crystalTransform.translation_ = {pillar.x, pillar.crystalY, pillar.z};
			pillar.crystalTransform.rotation_ = {0.0f, 0.0f, 0.0f}; // ★ 回転させない
			pillar.crystalTransform.scale_ = {1.2f, 1.2f, 1.2f};
			pillar.crystalTransform.matWorld_ = MakeAffineMatrix(pillar.crystalTransform.scale_, pillar.crystalTransform.rotation_, pillar.crystalTransform.translation_);
			pillar.crystalTransform.TransferMatrix();

			if (pillar.timer >= pillar.telegraphDuration) {
				pillar.status = DoomPillarStatus::kStrike;
				pillar.timer = 0.0f;
				struckNow = true;
				EmitDoomSparks(pillar.x, pillar.groundY);
				if (distToPlayer < nearestStrikeDist) {
					nearestStrikeDist = distToPlayer;
					doomStrikeEventPos_ = {pillar.x, pillar.groundY + 2.0f, 0.0f};
				}
			}
		}
		// ★ 3. 着弾・判定発生
		else if (pillar.status == DoomPillarStatus::kStrike) {
			const float k = std::clamp(pillar.timer / kDoomStrikeDuration, 0.0f, 1.0f);
			targetIntensity = (std::max)(targetIntensity, 1.0f - 0.4f * k);

			pillar.crystalY = pillar.targetY; // 空中静止のまま強力照射

			pillar.crystalTransform.translation_ = {pillar.x, pillar.crystalY, pillar.z};
			pillar.crystalTransform.rotation_ = {0.0f, 0.0f, 0.0f}; // ★ 回転させない
			pillar.crystalTransform.scale_ = {1.2f, 1.2f, 1.2f};
			pillar.crystalTransform.matWorld_ = MakeAffineMatrix(pillar.crystalTransform.scale_, pillar.crystalTransform.rotation_, pillar.crystalTransform.translation_);
			pillar.crystalTransform.TransferMatrix();

			// ★ 照射枠と完全一致する当たり判定処理
			if (!pillar.hitDone && pillar.timer >= 0.02f && player_ && !player_->IsDead()) {
				const Player::AABB playerAABB = player_->GetAABB();
				const float pX = player_->GetWorldTransform().translation_.x;

				const float pillarMinX = pillar.x - pillarHalfWidth;
				const float pillarMaxX = pillar.x + pillarHalfWidth;
				const float pillarMinY = pillar.groundY;
				const float pillarMaxY = pillar.groundY + 25.0f;

				const bool hitX = (pX + 0.15f > pillarMinX) && (pX - 0.15f < pillarMaxX);
				const bool hitY = (playerAABB.max.y > pillarMinY) && (playerAABB.min.y < pillarMaxY);

				if (hitX && hitY) {
					pillar.hitDone = true;
					player_->OnCollision();
				}
			}

			if (pillar.timer >= kDoomStrikeDuration) {
				pillar.status = DoomPillarStatus::kReturning; // ★ 射線が消えて上に戻るフェーズへ
				pillar.timer = 0.0f;
			}
		}
		// ★ 4. 画像の射線が消え、クリスタルが上空へ戻っていく
		else if (pillar.status == DoomPillarStatus::kReturning) {
			const float startY = pillar.targetY;
			const float targetY = pillar.groundY + 18.0f;

			float t = std::clamp(pillar.timer / kDoomReturnDuration, 0.0f, 1.0f);
			float easeT = t * t;
			pillar.crystalY = startY + (targetY - startY) * easeT;

			pillar.crystalTransform.translation_ = {pillar.x, pillar.crystalY, pillar.z};
			pillar.crystalTransform.rotation_ = {0.0f, 0.0f, 0.0f}; // ★ 回転させない
			pillar.crystalTransform.scale_ = {1.2f, 1.2f, 1.2f};
			pillar.crystalTransform.matWorld_ = MakeAffineMatrix(pillar.crystalTransform.scale_, pillar.crystalTransform.rotation_, pillar.crystalTransform.translation_);
			pillar.crystalTransform.TransferMatrix();

			if (pillar.timer >= kDoomReturnDuration) {
				pillar.status = DoomPillarStatus::kInactive;
			}
		}

		if (pillar.status == DoomPillarStatus::kStrike && distToPlayer < nearestLightDist) {
			nearestLightDist = distToPlayer;
			doomLightPos_ = {pillar.x, pillar.groundY + 2.5f, 0.0f};
		}
	}

	doomStrikeIntensity_ += (targetIntensity - doomStrikeIntensity_) * 0.25f;
	if (targetIntensity <= 0.0f && doomStrikeIntensity_ < 0.005f) {
		doomStrikeIntensity_ = 0.0f;
	}

	if (struckNow) {
		doomPunch_ = 1.0f;
		doomStrikeEvent_ = true;
		AudioManager::GetInstance()->PlaySE(SEType::kBossAttack); // ★ 柱着弾SE再生
	}

	// ★ 規定の回数（kDoomTotalCount）終了し、画面上の柱が全て戻り切ったら流星群へ移行
	if (doomCount_ >= kDoomTotalCount && !anyAlive) {
		doomStrikeIntensity_ = 0.0f;
		frenzyWaitDuration_ = isAwakened_ ? 2.0f : 6.0f; // ★ 覚醒時のインターバルを保持
		StartAttack();                                   // ★ 次の流星群攻撃へ状態切り替え
	}
}

// ★ 覚醒演出・終末の裁きの本体姿勢
void FinalBoss::ApplyAwakenBodyPose() {
	const float pi = std::numbers::pi_v<float>;

	if (state_ == State::kAwakening) {
		const float t = std::clamp(awakeningTimer_ / kAwakeningDuration, 0.0f, 1.0f);
		const float punch = std::sin(std::clamp((t - 0.62f) / 0.16f, 0.0f, 1.0f) * pi);
		const float settle = SmoothStep(0.78f, 1.0f, t);
		const float tremble = (0.03f + 0.20f * SmoothStep(0.18f, 0.62f, t)) * (1.0f - SmoothStep(0.70f, 0.90f, t));

		float y = LerpF(-0.8f, 2.0f, SmoothStep(0.15f, 0.62f, t));
		y = LerpF(y, 1.2f, settle) + 0.5f * punch;

		const float lean = awakenStartRotX_ * (1.0f - SmoothStep(0.05f, 0.40f, t)) - 0.25f * SmoothStep(0.40f, 0.62f, t) * (1.0f - settle) + 0.35f * punch;
		const float scale = 1.0f + 0.15f * SmoothStep(0.20f, 1.0f, t) + 0.25f * punch;

		worldTransformBody_.translation_.x = std::sin(animTimer_ * 38.0f) * tremble;
		worldTransformBody_.translation_.y = y + std::cos(animTimer_ * 33.0f) * tremble;
		worldTransformBody_.rotation_.x = lean;
		worldTransformBody_.rotation_.z = std::sin(animTimer_ * 21.0f) * 0.5f * tremble;
		worldTransformBody_.scale_ = {scale, scale, scale};
		return;
	}

	if (state_ == State::kDodgingAttack) {
		const float chargeT = std::clamp(dodgingAttackTimer_ / kDoomChargeDuration, 0.0f, 1.0f);
		const float charge = SmoothStep(0.0f, 1.0f, chargeT);

		float y = LerpF(1.2f, 2.0f, charge) + 0.25f * std::sin(animTimer_ * 2.2f) * charge;
		float scale = LerpF(1.15f, 1.3f, charge) + 0.12f * doomPunch_;

		worldTransformBody_.translation_.x = 0.0f;
		worldTransformBody_.translation_.y = y;
		worldTransformBody_.rotation_.x = -0.2f * charge;
		worldTransformBody_.rotation_.z = 0.0f;
		worldTransformBody_.scale_ = {scale, scale, scale};
	}
}

void FinalBoss::BeginExecution() {
	isExecuting_ = true;
	executionStruck_ = false;
	executionCharge_ = 0.0f;
	executionSpin_ = 0.0f;
	executionStrikeTimer_ = 0.0f;
}

void FinalBoss::ExecutionStrike(float directionX) {
	executionStruck_ = true;
	executionStrikeTimer_ = 0.0f;
	executionStrikeDirX_ = directionX;
	executionCharge_ = 1.0f;

	for (int i = 0; i < 10; ++i) {
		EmitDefeatParticles();
	}
}

void FinalBoss::EndExecution() {
	isExecuting_ = false;
	crystalHidden_.fill(true);
}

void FinalBoss::Update() {
	if (isDead_ || state_ == State::kStandby) {
		return;
	}

	const float dt = 1.0f / 60.0f;
	animTimer_ += dt;
	if (state_ == State::kAttack) {
		attackTimer_ += dt;
	}

	if (state_ == State::kSpawn) {
		spawnTimer_ += dt;
		float progress = std::min(spawnTimer_ / spawnDuration_, 1.0f);

		EmitSpawnParticles();

		float easeProgress = progress * progress * (3.0f - 2.0f * progress);
		worldTransform_.scale_ = {targetScale_.x * easeProgress, targetScale_.y * easeProgress, targetScale_.z * easeProgress};

		float shake = (1.0f - progress) * 0.15f;
		worldTransformBody_.translation_.x = std::sin(animTimer_ * 40.0f) * shake;
		worldTransformBody_.translation_.y = std::cos(animTimer_ * 35.0f) * shake;

		if (spawnTimer_ >= spawnDuration_) {
			worldTransform_.scale_ = targetScale_;
			state_ = State::kFrenzyInit;
			frenzyTimer_ = 0.0f;
		}
	}

	UpdateParticles();

	if (isExecuting_) {
		if (!executionStruck_ && executionCharge_ > 0.05f) {
			EmitSpawnParticles();
		} else if (executionStruck_ && executionStrikeTimer_ < 0.5f) {
			EmitDefeatParticles();
		}
		executionSpin_ += dt * (0.6f + 8.0f * executionCharge_ * executionCharge_);
	}

	if (state_ == State::kFrenzyInit) {
		frenzyTimer_ += dt;
		if (frenzyTimer_ >= 2.0f) {
			state_ = State::kFrenzy;
		}
	}

	if (state_ == State::kFrenzy) {
		frenzyWaitTimer_ += dt;
		if (frenzyWaitTimer_ >= frenzyWaitDuration_ && !isFinalBossDefeated_) {
			if (isAwakened_) {
				StartDodgingAttack();
			} else {
				StartAttack();
			}
		}
	}

	awakenRoarGlow_ = (std::max)(0.0f, awakenRoarGlow_ - dt * 0.8f);

	if (state_ == State::kAwakening) {
		awakeningTimer_ += dt;
		const float t = std::clamp(awakeningTimer_ / kAwakeningDuration, 0.0f, 1.0f);

		if (t >= 0.20f && t < 0.62f && (rand() % 2) == 0) {
			EmitSpawnParticles();
		}

		if (!awakenRoared_ && t >= 0.62f) {
			awakenRoared_ = true;
			awakenRoarGlow_ = 1.0f;
			AudioManager::GetInstance()->PlaySE(SEType::kBossSkillActive); // ★ ラスボススキル発動時（咆哮）SE再生
			for (int i = 0; i < 8; ++i) {
				EmitDefeatParticles();
			}
		}

		if (awakeningTimer_ >= kAwakeningDuration) {
			StartDodgingAttack();
		}
	}

	if (state_ == State::kDodgingAttack) {
		UpdateDodgingAttack(dt);
	}

	if (state_ == State::kAttack) {
		spawnIntervalTimer_ += dt;

		if (currentAttackIndex_ < attackOrder_.size() && spawnIntervalTimer_ >= spawnInterval_) {
			spawnIntervalTimer_ = 0.0f;
			size_t idx = attackOrder_[currentAttackIndex_++];
			CrystalAttackInfo& info = crystalAttackInfos_[idx];
			info.status = CrystalAttackStatus::kLaunching;
			info.timer = 0.0f;
			info.startPosition = {worldTransformCrystals_[idx].matWorld_.m[3][0], worldTransformCrystals_[idx].matWorld_.m[3][1], worldTransformCrystals_[idx].matWorld_.m[3][2]};
			info.currentWorldPos = info.startPosition;
		}

		for (size_t i = 0; i < kMeteorCrystals; ++i) {
			CrystalAttackInfo& info = crystalAttackInfos_[i];
			if (info.status == CrystalAttackStatus::kLaunching) {
				info.timer += dt;
				info.currentWorldPos.y += 0.55f;
				if (info.timer >= 0.55f) {
					info.status = CrystalAttackStatus::kIdle;
					crystalHidden_[i] = true;
				}
			}
		}

		bool allLaunched = (currentAttackIndex_ >= attackOrder_.size());
		if (allLaunched) {
			for (size_t i = 0; i < kMeteorCrystals; ++i) {
				if (!crystalHidden_[i]) {
					allLaunched = false;
					break;
				}
			}
		}

		if (allLaunched && !meteorShowerStarted_) {
			meteorShowerStarted_ = true;
			AudioManager::GetInstance()->PlaySE(SEType::kBossSkillActive); // ★ ラスボススキル発動時SE再生
			for (size_t i = 0; i < kNumMeteorShower; ++i) {
				meteorCrystals_[i].active = true;
			}
		}

		bool allMeteorsFinished = meteorShowerStarted_;
		if (meteorShowerStarted_) {
			for (size_t i = 0; i < kNumMeteorShower; ++i) {
				auto& meteor = meteorCrystals_[i];
				if (!meteor.active) {
					continue;
				}

				allMeteorsFinished = false;

				if (meteor.delay > 0.0f) {
					meteor.delay -= dt;
					continue;
				}

				meteor.timer += dt;

				const float gravity = 0.007f;
				meteor.velocity.y -= gravity;

				meteor.position.x += meteor.velocity.x;
				meteor.position.y += meteor.velocity.y;

				meteor.worldTransform.translation_ = meteor.position;
				meteor.worldTransform.rotation_.z += 0.12f;

				meteor.worldTransform.scale_ = meteor.scale;
				meteor.worldTransform.matWorld_ = MakeAffineMatrix(meteor.worldTransform.scale_, meteor.worldTransform.rotation_, meteor.worldTransform.translation_);
				meteor.worldTransform.TransferMatrix();

				if (meteor.bounceCount == 0 && player_ && !player_->IsDead()) {
					Player::AABB playerAABB = player_->GetAABB();
					float marginX = 0.15f;
					float marginY = 0.15f;

					if (meteor.position.x >= playerAABB.min.x - marginX && meteor.position.x <= playerAABB.max.x + marginX && meteor.position.y >= playerAABB.min.y - marginY &&
					    meteor.position.y <= playerAABB.max.y + marginY) {

						player_->OnCollision();
						meteor.active = false;
						AudioManager::GetInstance()->PlaySE(SEType::kAttack);
						continue;
					}
				}

				if (mapChipField_) {
					MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(meteor.position);
					MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.x, index.y);
					bool isGroundBlock = (type == MapChipType::kBlock && mapChipField_->GetBlockFaceByIndex(index.x, index.y) == BlockFace::kAbove);
					bool isFloatingTile = (type == MapChipType::kBlockFall);

					if (isGroundBlock || isFloatingTile) {
						if (meteor.velocity.y < 0.0f) {
							if (meteor.bounceCount < meteor.maxBounces) {
								meteor.velocity.y = -meteor.velocity.y * 0.5f;
								meteor.velocity.x *= 0.7f;
								KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);
								meteor.position.y = blockPos.y + 0.5f;
								meteor.bounceCount++;
								AudioManager::GetInstance()->PlaySE(SEType::kAttack);
							} else {
								meteor.active = false;
								continue;
							}
						}
					}
				}

				if (meteor.position.y < basePosition_.y - 12.0f) {
					meteor.active = false;
				}
			}
		}

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
					worldTransformCrystal22_.translation_.y = blockPos.y + 0.5f;
					crystal22Landed_ = true;
					crystal22Timer_ = 0.0f;
					crystal22LandedTimer_ = 0.0f;
				}
			}

			if (t >= 1.0f && !crystal22Landed_) {
				crystal22Landed_ = true;
				crystal22Timer_ = 0.0f;
				crystal22LandedTimer_ = 0.0f;
			}
		}

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
				attackDamageDone_ = true;
				finalBossHp_ = std::max(0, finalBossHp_ - 1);
				hitReactionTimer_ = 0.0f;
				hitReactionRotation_ = 0.35f;
				crystal22HitCount_ = 0;
				crystal22Timer_ = 0.0f;
				worldTransformCrystal22_.translation_ = {0.0f, -100.0f, 0.0f};

				if (finalBossHp_ > 0) {
					pendingDown_ = true;
				}

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
					if (!isAwakened_) {
						TriggerAwakening();
					} else {
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
		}

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

		if (attackDamageDone_ && allReturned && !crystal22Landed_ && !crystal22Flying_ && currentAttackIndex_ >= attackOrder_.size()) {
			if (!pendingDown_) {
				state_ = isFinalBossDefeated_ ? State::kDefeat : State::kFrenzy;
				frenzyWaitTimer_ = 0.0f;
				frenzyWaitDuration_ = isAwakened_ ? 2.0f : 6.0f; // ★ 覚醒時の攻撃インターバルを2.0秒に短縮
				crystal22Timer_ = 0.0f;
			}
		}
	}

	if (state_ == State::kDownWarning) {
		downWarningTimer_ += dt;

		float progress = downWarningTimer_ / kDownWarningDuration;
		worldTransformBody_.translation_.x = std::sin(downWarningTimer_ * 35.0f) * 0.35f;
		worldTransformBody_.translation_.y = std::cos(downWarningTimer_ * 28.0f) * 0.25f - 0.5f * progress;
		worldTransformBody_.rotation_.z = std::sin(downWarningTimer_ * 20.0f) * 0.25f;
		worldTransformBody_.rotation_.x = 0.4f * SmoothStep(0.0f, 1.0f, progress);

		if (rand() % 2 == 0) {
			EmitDefeatParticles();
		}

		if (downWarningTimer_ >= kDownWarningDuration) {
			state_ = State::kDown;
			downTimer_ = 0.0f;
			downStartPosition_ = worldTransform_.translation_;

			KamataEngine::Vector3 targetPos = basePosition_;
			if (player_) {
				targetPos.x = player_->GetWorldTransform().translation_.x;
				targetPos.y = player_->GetWorldTransform().translation_.y - 1.2f;
			} else {
				targetPos.y -= 4.0f;
			}
			targetPos.z = basePosition_.z;
			downTargetPosition_ = targetPos;

			AudioManager::GetInstance()->PlaySE(SEType::kBossDown); // ★ 追加: 地面に伏せる（ダウン）時SE再生
		}
	}

	if (state_ == State::kDown) {
		downTimer_ += dt;

		const float fallInDuration = 0.6f;
		const float riseDuration = 0.8f;
		const float stayDuration = downDuration_ - fallInDuration - riseDuration;

		if (downTimer_ <= fallInDuration) {
			float t = std::min(downTimer_ / fallInDuration, 1.0f);
			float easeT = SmoothStep(0.0f, 1.0f, t);

			worldTransform_.translation_ = {
			    downStartPosition_.x + (downTargetPosition_.x - downStartPosition_.x) * easeT, downStartPosition_.y + (downTargetPosition_.y - downStartPosition_.y) * easeT,
			    downStartPosition_.z + (downTargetPosition_.z - downStartPosition_.z) * easeT};

			worldTransformBody_.rotation_.x = 1.25f * easeT;
			worldTransformBody_.rotation_.z = 0.15f * std::sin(downTimer_ * 10.0f);

		} else if (downTimer_ <= fallInDuration + stayDuration) {
			worldTransform_.translation_ = downTargetPosition_;
			worldTransformBody_.rotation_.x = 1.25f;
			worldTransformBody_.rotation_.z = 0.05f * std::sin(animTimer_ * 6.0f);

		} else if (downTimer_ <= downDuration_) {
			float t = std::min((downTimer_ - fallInDuration - stayDuration) / riseDuration, 1.0f);
			float easeT = SmoothStep(0.0f, 1.0f, t);

			worldTransform_.translation_ = {
			    downTargetPosition_.x + (basePosition_.x - downTargetPosition_.x) * easeT, downTargetPosition_.y + (basePosition_.y - downTargetPosition_.y) * easeT,
			    downTargetPosition_.z + (basePosition_.z - downTargetPosition_.z) * easeT};

			worldTransformBody_.rotation_.x = 1.25f * (1.0f - easeT);
			worldTransformBody_.rotation_.z = 0.0f;

		} else {
			worldTransform_.translation_ = basePosition_;
			worldTransformBody_.rotation_ = {0.0f, 0.0f, 0.0f};
			state_ = isFinalBossDefeated_ ? State::kDefeat : State::kFrenzy;
			frenzyWaitTimer_ = 0.0f;
			frenzyWaitDuration_ = isAwakened_ ? 2.0f : 6.0f; // ★ 覚醒時の攻撃インターバルを2.0秒に短縮
			downTimer_ = 0.0f;
		}
	}

	if (state_ == State::kDefeat) {
		if (isDefeatZoomFinished_) {
			if (defeatTimer_ == 0.0f) {
				AudioManager::GetInstance()->PlaySE(SEType::kBossHpZero); // ★ 追加: 撃破アニメーション（痙攣）開始時SE再生
			}

			defeatTimer_ += dt;
			const float defeatProgress = std::min(defeatTimer_ / kDefeatDuration, 1.0f);

			if (defeatProgress < 0.3f) {
				if (rand() % 3 == 0) {
					EmitDefeatParticles();
				}
			} else if (defeatProgress >= 0.8f) {
				EmitDefeatParticles();
			}

			const float shrink = SmoothStep(0.8f, 1.0f, defeatProgress);
			const float scaleRate = 1.0f - shrink;
			worldTransform_.scale_ = {targetScale_.x * scaleRate, targetScale_.y * scaleRate, targetScale_.z * scaleRate};

			if (defeatTimer_ >= kDefeatDuration) {
				worldTransform_.scale_ = {0.0f, 0.0f, 0.0f};
				isDefeatAnimationFinished_ = true;
				state_ = State::kNormal;
				AudioManager::GetInstance()->PlaySE(SEType::kBossDisappear); // ★ ラスボスが消える時SE再生
			}
		}
	}

	if (state_ != State::kDown && state_ != State::kDownWarning && camera_) {
		worldTransform_.translation_ = GetAnchoredPosition();

		worldTransform_.translation_.x += awakenRiseOffset_.x;
		worldTransform_.translation_.y += awakenRiseOffset_.y;
		worldTransform_.translation_.z += awakenRiseOffset_.z;
		awakenRiseOffset_ = {awakenRiseOffset_.x * 0.95f, awakenRiseOffset_.y * 0.95f, awakenRiseOffset_.z * 0.95f};
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (state_ != State::kDown && state_ != State::kDownWarning && state_ != State::kAwakening && state_ != State::kDodgingAttack) {
		worldTransformBody_.translation_ = {0.0f, 0.0f, 0.0f};
		worldTransformBody_.rotation_ = {0.0f, 0.0f, 0.0f};
		worldTransformBody_.scale_ = {1.0f, 1.0f, 1.0f};
	}

	if (state_ == State::kFrenzyInit) {
		float shakeAmount = 0.25f;
		worldTransformBody_.translation_.x = std::sin(frenzyTimer_ * 50.0f) * shakeAmount;
		worldTransformBody_.translation_.y = std::cos(frenzyTimer_ * 40.0f) * shakeAmount;
		worldTransformBody_.rotation_.z = std::sin(frenzyTimer_ * 35.0f) * 0.1f;
	}

	if (hitReactionTimer_ < 0.45f && hitReactionRotation_ != 0.0f) {
		hitReactionTimer_ += dt;
		worldTransformBody_.rotation_.z = hitReactionRotation_ * (1.0f - std::min(hitReactionTimer_ / 0.45f, 1.0f));
	} else {
		worldTransformBody_.rotation_.z = 0.0f;
		hitReactionRotation_ = 0.0f;

		if (pendingDown_ && state_ != State::kDefeat) {
			pendingDown_ = false;
			state_ = State::kDownWarning;
			downWarningTimer_ = 0.0f;
			bodyHitConsumed_ = false;
			AudioManager::GetInstance()->PlaySE(SEType::kBossDown); // ★ 伏せ（ダウン）予告開始SE再生
		}
	}

	if (state_ == State::kAttack) {
		const float introT = std::clamp(attackTimer_ / kAttackIntroDuration, 0.0f, 1.0f);
		float charge = 0.0f;
		float roar = 0.0f;
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

		if (attackTimer_ >= kAttackIntroDuration) {
			bodyScale *= 1.0f + 0.04f * std::sin(animTimer_ * 3.0f);
			worldTransformBody_.rotation_.z += 0.07f * std::sin(animTimer_ * 1.6f);
			worldTransformBody_.translation_.y += 0.25f * std::sin(animTimer_ * 2.2f);
		}
		worldTransformBody_.scale_ = {bodyScale, bodyScale, bodyScale};
	}

	if (isExecuting_) {
		if (!executionStruck_) {
			const float tremble = 0.02f + 0.16f * executionCharge_;
			worldTransformBody_.translation_.x += std::sin(animTimer_ * 70.0f) * tremble;
			worldTransformBody_.translation_.y += std::cos(animTimer_ * 63.0f) * tremble + 1.0f * executionCharge_;
			worldTransformBody_.rotation_.z += std::sin(animTimer_ * 45.0f) * 0.07f * executionCharge_;
			const float pulse = 1.0f + 0.12f * executionCharge_ * std::sin(animTimer_ * 14.0f);
			worldTransformBody_.scale_ = {pulse, pulse, pulse};
		} else {
			executionStrikeTimer_ += dt;
			const float t = std::clamp(executionStrikeTimer_ / 0.6f, 0.0f, 1.0f);
			const float impact = (1.0f - t) * (1.0f - t);
			const float sag = SmoothStep(0.0f, 1.0f, t);
			worldTransformBody_.translation_.x += executionStrikeDirX_ * 2.0f * impact + std::sin(animTimer_ * 30.0f) * 0.03f * sag;
			worldTransformBody_.translation_.y += -1.0f * sag;
			worldTransformBody_.rotation_.z += -executionStrikeDirX_ * (0.9f * impact + 0.25f * sag);
			worldTransformBody_.scale_ = {1.0f + 0.3f * impact, 1.0f - 0.3f * impact, 1.0f + 0.3f * impact};
		}
	}

	if (state_ == State::kDefeat && isDefeatZoomFinished_) {
		const float defeatProgress = std::min(defeatTimer_ / kDefeatDuration, 1.0f);
		const float convulsion = 1.0f - SmoothStep(0.0f, 0.5f, defeatProgress);
		const float fall = SmoothStep(0.25f, 0.85f, defeatProgress);

		worldTransformBody_.translation_.x += std::sin(defeatTimer_ * 55.0f) * 0.45f * convulsion;
		worldTransformBody_.translation_.y += std::cos(defeatTimer_ * 47.0f) * 0.35f * convulsion - 3.0f * fall * fall;
		worldTransformBody_.rotation_.z += std::sin(defeatTimer_ * 38.0f) * 0.12f * convulsion + 0.9f * fall;

		worldTransformBody_.scale_ = {1.0f, 1.0f, 1.0f};
	}

	ApplyAwakenBodyPose();

	KamataEngine::Matrix4x4 localMatBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatBody, worldTransform_.matWorld_);
	worldTransformBody_.TransferMatrix();

	float handSpeed = (state_ == State::kAttack || state_ == State::kDodgingAttack) ? 6.0f : ((state_ == State::kFrenzy || isAwakened_) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f));
	float handSpread = (state_ == State::kAttack || state_ == State::kDodgingAttack)
	                       ? (std::min(attackTimer_ / kAttackIntroDuration, 1.0f) * 2.4f + std::sin(animTimer_ * 4.0f) * 0.2f)
	                       : ((state_ == State::kFrenzyInit) ? (std::min(frenzyTimer_ / 2.0f, 1.0f) * 1.8f) : ((state_ == State::kFrenzy) ? 1.5f : 0.0f));

	if (state_ == State::kAwakening) {
		const float t = GetAwakeningProgress();
		handSpread = 0.4f + 2.0f * SmoothStep(0.2f, 0.62f, t) * (1.0f - 0.5f * SmoothStep(0.62f, 1.0f, t));
		handSpeed = 3.0f + 9.0f * SmoothStep(0.2f, 0.62f, t);
	}

	if (state_ == State::kDodgingAttack) {
		const float charge = SmoothStep(0.0f, 1.0f, std::clamp(dodgingAttackTimer_ / kDoomChargeDuration, 0.0f, 1.0f));
		handSpread = 1.5f + 1.2f * charge + 0.3f * doomPunch_;
		handSpeed = 6.0f;
	}

	if (state_ == State::kDefeat) {
		const float handFall = SmoothStep(0.25f, 0.85f, std::min(defeatTimer_ / kDefeatDuration, 1.0f));
		handSpread = 1.5f * (1.0f - handFall);
		handSpeed = 6.0f * (1.0f - handFall) + 0.8f;
	}

	if (isExecuting_) {
		handSpread = executionStruck_ ? 0.3f : 1.5f * (1.0f - 0.7f * executionCharge_);
		handSpeed = executionStruck_ ? 1.5f : 6.0f + 12.0f * executionCharge_;
	}

	for (size_t i = 0; i < kNumHands; ++i) {
		float phaseOffset = static_cast<float>(i) * 0.8f;

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

	float baseCrystalSpeed = (state_ == State::kAttack || state_ == State::kDodgingAttack) ? 6.0f : ((state_ == State::kFrenzy || isAwakened_) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f));
	float crystalAmp = (state_ == State::kAttack || state_ == State::kDodgingAttack) ? 1.0f : ((state_ == State::kFrenzy || isAwakened_) ? 1.2f : 0.6f);

	for (size_t i = 0; i < kNumCrystals; ++i) {
		float phaseOffset = static_cast<float>(i) * 0.7f;

		float speedMultiplier = 0.6f + static_cast<float>((i * 5 + 3) % 11) * 0.15f;
		float currentSpeed = baseCrystalSpeed * speedMultiplier;

		float verticalOffset = std::sin(animTimer_ * currentSpeed + phaseOffset) * crystalAmp + std::sin(animTimer_ * (currentSpeed * 0.63f) + phaseOffset * 1.5f) * (crystalAmp * 0.35f);

		worldTransformCrystals_[i].translation_.x = crystalLocalOffsets_[i].x;
		worldTransformCrystals_[i].translation_.y = crystalLocalOffsets_[i].y + verticalOffset;
		worldTransformCrystals_[i].translation_.z = crystalLocalOffsets_[i].z;

		worldTransformCrystals_[i].rotation_ = {0.0f, 0.0f, 0.0f};

		if (state_ == State::kDefeat) {
			const float scatter = SmoothStep(0.2f, 0.9f, std::min(defeatTimer_ / kDefeatDuration, 1.0f));
			const float dirX = crystalLocalOffsets_[i].x;
			const float dirY = crystalLocalOffsets_[i].y;
			const float len = std::sqrt(dirX * dirX + dirY * dirY) + 0.001f;
			worldTransformCrystals_[i].translation_.x += dirX / len * scatter * 8.0f;
			worldTransformCrystals_[i].translation_.y += dirY / len * scatter * 4.0f - scatter * scatter * 10.0f;
			worldTransformCrystals_[i].rotation_.z = scatter * (static_cast<float>(i % 5) - 2.0f) * 3.0f;
		}

		if (isExecuting_ && i < kMeteorCrystals) {
			const float spinDir = (i % 2 == 0) ? 1.0f : -1.0f;
			float crystalScale = 1.0f;
			if (!executionStruck_) {
				crystalScale = 1.0f - 0.55f * executionCharge_;
			} else {
				const float shatter = std::clamp(executionStrikeTimer_ / kExecutionShatterDuration, 0.0f, 1.0f);
				crystalScale = 0.45f + 2.5f * shatter;
				if (shatter >= 1.0f) {
					crystalHidden_[i] = true;
				}
			}
			worldTransformCrystals_[i].scale_ = {crystalScale, crystalScale, crystalScale};
			worldTransformCrystals_[i].rotation_.z = spinDir * executionSpin_ * (1.0f + 0.1f * static_cast<float>(i % 5));
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

	worldTransformCrystal22_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransformCrystal22_.rotation_ = {0.0f, 0.0f, animTimer_ * 2.0f};
	worldTransformCrystal22_.matWorld_ = MakeAffineMatrix(worldTransformCrystal22_.scale_, worldTransformCrystal22_.rotation_, worldTransformCrystal22_.translation_);
	worldTransformCrystal22_.TransferMatrix();
}

std::vector<KamataEngine::Vector3> FinalBoss::GetActiveMeteorPositions() const {
	std::vector<KamataEngine::Vector3> positions;
	if (state_ != State::kAttack) {
		return positions;
	}

	for (size_t i = 0; i < kNumMeteorShower; ++i) {
		if (meteorCrystals_[i].active && meteorCrystals_[i].delay <= 0.0f) {
			positions.push_back(meteorCrystals_[i].worldTransform.translation_);
		}
	}

	if (!crystal22Flying_ && (crystal22Landed_ || crystal22Timer_ > 0.0f)) {
		positions.push_back(worldTransformCrystal22_.translation_);
	}

	return positions;
}

void FinalBoss::DrawDoomPillars(const KamataEngine::Matrix4x4& matVP) {
	auto drawRect = [this](float centerX, float centerY, float width, float height, const KamataEngine::Vector4& color) {
		spriteTargetMarker_->SetPosition({centerX, centerY});
		spriteTargetMarker_->SetSize({width, height});
		spriteTargetMarker_->SetColor(color);
		spriteTargetMarker_->Draw();
	};

	const float pillarHalfWidth = kDoomPillarHalfWidth;

	for (const auto& pillar : doomPillars_) {
		// ★ kTelegraph と kStrike の時だけ射線（照射ビーム）を描画（kReturning では射線が消える）
		if (pillar.status != DoomPillarStatus::kTelegraph && pillar.status != DoomPillarStatus::kStrike) {
			continue;
		}

		// ★ 地面のスクリーン座標
		const KamataEngine::Vector2 groundScreen = ProjectToScreen({pillar.x, pillar.groundY, 0.0f}, matVP);
		// ★ 空中静止しているクリスタルの先端（下端）の3D座標とスクリーン座標
		const KamataEngine::Vector3 crystalTip3D = {pillar.x, pillar.crystalY - 1.2f, pillar.z};
		const KamataEngine::Vector2 crystalTipScreen = ProjectToScreen(crystalTip3D, matVP);

		// ★ 判定領域の左右幅ピクセル
		const KamataEngine::Vector2 left = ProjectToScreen({pillar.x - pillarHalfWidth, pillar.groundY, 0.0f}, matVP);
		const KamataEngine::Vector2 right = ProjectToScreen({pillar.x + pillarHalfWidth, pillar.groundY, 0.0f}, matVP);

		if (groundScreen.x < -9000.0f || crystalTipScreen.x < -9000.0f) {
			continue;
		}

		const float widthPx = std::fabs(right.x - left.x);
		if (groundScreen.x + widthPx < -100.0f || groundScreen.x - widthPx > 1380.0f) {
			continue;
		}

		if (pillar.status == DoomPillarStatus::kTelegraph) {
			const float k = std::clamp(pillar.timer / pillar.telegraphDuration, 0.0f, 1.0f);
			const float ease = k * k;

			// ★ クリスタル下端から地面までのY中心と高さ（長さを動的に算出）
			const float rayTopY = (std::min)(crystalTipScreen.y, groundScreen.y);
			const float rayBottomY = (std::max)(crystalTipScreen.y, groundScreen.y);
			const float rayCenterY = (rayTopY + rayBottomY) * 0.5f;
			const float rayHeightPx = std::fabs(rayBottomY - rayTopY);

			// ★ 床の目標マーカー（着弾危険枠）
			drawRect(groundScreen.x, groundScreen.y, widthPx * 1.2f, 16.0f + 8.0f * k, {1.0f, 0.22f, 0.08f, 0.40f + 0.40f * ease});

			// ★ クリスタルの先端（下端）から地面へまっすぐ伸びる画像射線（予告照射ビーム）
			drawRect(groundScreen.x, rayCenterY, widthPx * 0.8f, rayHeightPx, {1.0f, 0.20f, 0.08f, 0.15f + 0.25f * ease}); // 射線の外光
			drawRect(groundScreen.x, rayCenterY, widthPx * 0.3f, rayHeightPx, {1.0f, 0.80f, 0.30f, 0.40f + 0.50f * ease}); // 射線のコア光線
		} else if (pillar.status == DoomPillarStatus::kStrike) {
			const float k = std::clamp(pillar.timer / kDoomStrikeDuration, 0.0f, 1.0f);
			const float grow = SmoothStep(0.0f, 0.06f, pillar.timer);
			const float a = 1.0f - 0.35f * k;

			const KamataEngine::Vector2 topScreen = ProjectToScreen({pillar.x, pillar.groundY + 26.0f, 0.0f}, matVP);
			const float yTop = (std::max)(topScreen.y, -60.0f);
			const float yBottom = (std::min)(groundScreen.y, 780.0f);
			const float centerY = (yTop + yBottom) * 0.5f;
			const float heightPx = yBottom - yTop;

			drawRect(groundScreen.x, centerY, widthPx * 1.2f * grow, heightPx, {1.0f, 0.16f, 0.05f, 0.25f * a});
			drawRect(groundScreen.x, centerY, widthPx * grow, heightPx, {1.0f, 0.42f, 0.18f, 0.50f * a});
			drawRect(groundScreen.x, centerY, widthPx * 0.35f * grow, heightPx, {1.0f, 0.85f, 0.65f, 0.65f * a});
			drawRect(groundScreen.x, groundScreen.y, widthPx * 1.3f * grow, 18.0f, {1.0f, 0.60f, 0.30f, 0.60f * a});
		}
	}
}

void FinalBoss::DrawTargetMarkers() {
	if (!spriteTargetMarker_ || !camera_ || (state_ != State::kAttack && state_ != State::kDodgingAttack)) {
		return;
	}

	const KamataEngine::Matrix4x4 matVP = MultiplyMatrix(camera_->matView, camera_->matProjection);

	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();
	if (dxCommon) {
		KamataEngine::Sprite::PreDraw(dxCommon->GetCommandList(), KamataEngine::Sprite::BlendMode::kAdd);
	} else {
		KamataEngine::Sprite::PreDraw();
	}

	if (state_ == State::kDodgingAttack) {
		DrawDoomPillars(matVP);
	}

	if (meteorShowerStarted_) {
		for (size_t i = 0; i < kNumMeteorShower; ++i) {
			const auto& meteor = meteorCrystals_[i];
			if (meteor.active && meteor.bounceCount == 0 && meteor.delay <= 0.0f) {
				KamataEngine::Vector2 screenPos = ProjectToScreen(meteor.targetPosition, matVP);
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

		KamataEngine::Vector2 screenPos = ProjectToScreen(info.targetPosition, matVP);
		if (screenPos.x < -100.0f || screenPos.x > 1380.0f || screenPos.y < -100.0f || screenPos.y > 820.0f) {
			continue;
		}

		float pulse = (std::sin(info.timer * 25.0f) * 0.5f + 0.5f);
		float alpha = 0.5f + pulse * 0.5f;

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

void FinalBoss::DrawParticles() {
	if (!spriteParticle_ || particles_.empty() || !camera_) {
		return;
	}

	const KamataEngine::Matrix4x4 matVP = MultiplyMatrix(camera_->matView, camera_->matProjection);

	KamataEngine::Sprite::PreDraw();
	for (const auto& p : particles_) {
		KamataEngine::Vector2 screenPos = ProjectToScreen(p.position, matVP);
		if (screenPos.x < -100.0f || screenPos.x > 1380.0f || screenPos.y < -100.0f || screenPos.y > 820.0f) {
			continue;
		}

		KamataEngine::Vector4 color = p.color;
		color.w *= (std::min)(1.0f, (1.0f - p.currentLife / p.maxLife) / 0.3f);

		spriteParticle_->SetPosition(screenPos);
		spriteParticle_->SetSize({p.scale.x * 200.0f, p.scale.y * 200.0f});
		spriteParticle_->SetColor(color);
		spriteParticle_->Draw();
	}
	KamataEngine::Sprite::PostDraw();
}

void FinalBoss::Draw() {
	if (isDead_ || !camera_ || state_ == State::kStandby) {
		return;
	}

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

		if (modelCrystal22_ && state_ == State::kAttack) {
			for (size_t i = 0; i < kNumMeteorShower; ++i) {
				if (meteorCrystals_[i].active && meteorCrystals_[i].delay <= 0.0f) {
					modelCrystal22_->Draw(meteorCrystals_[i].worldTransform, *camera_);
				}
			}
		}

		// ★ 覚醒攻撃「終末の裁き」で空中に停滞して照射する finalBossCrystal_22 モデルの描画
		if (modelCrystal22_ && state_ == State::kDodgingAttack) {
			for (const auto& pillar : doomPillars_) {
				if (pillar.status != DoomPillarStatus::kInactive) {
					modelCrystal22_->Draw(pillar.crystalTransform, *camera_);
				}
			}
		}

		if (modelCrystal22_ && (crystal22Landed_ || crystal22Flying_ || worldTransformCrystal22_.translation_.y > -50.0f)) {
			modelCrystal22_->Draw(worldTransformCrystal22_, *camera_);
		}
	}

	DrawTargetMarkers();

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

		if (isAwakened_) {
			spriteHpBar_->SetColor({0.95f, 0.05f, 0.15f, 1.0f});
		} else {
			spriteHpBar_->SetColor({1.0f, 0.12f, 0.18f, 1.0f});
		}

		spriteHpBar_->Draw();
		KamataEngine::Sprite::PostDraw();
	}

	DrawParticles();
}

void FinalBoss::OnCollision(Player* player) {
	(void)player;
	if (state_ == State::kDown && !bodyHitConsumed_ && !isFinalBossDefeated_) {
		finalBossHp_ = std::max(0, finalBossHp_ - 1);
		finalBossHitEvent_ = true;
		bodyHitConsumed_ = true;
		hitReactionTimer_ = 0.0f;
		hitReactionRotation_ = 0.25f;

		const float fallInDuration = 0.6f;
		const float riseDuration = 0.8f;
		const float stayDuration = downDuration_ - fallInDuration - riseDuration;
		if (downTimer_ < fallInDuration + stayDuration) {
			downTimer_ = fallInDuration + stayDuration;
		}

		if (finalBossHp_ <= 0) {
			if (!isAwakened_) {
				TriggerAwakening();
			} else {
				isFinalBossDefeated_ = true;
				state_ = State::kDefeat;
				defeatTimer_ = 0.0f;
			}
		}
	}
}

void FinalBoss::OnDead() { isDead_ = true; }

BaseEnemy::AABB FinalBoss::GetAABB() const {
	if (state_ == State::kDown) {
		KamataEngine::Vector3 pos = {worldTransformBody_.matWorld_.m[3][0], worldTransformBody_.matWorld_.m[3][1], worldTransformBody_.matWorld_.m[3][2]};
		float sizeX = 5.0f;
		float sizeY = 3.0f;
		float sizeZ = 5.0f;
		return AABB{
		    {pos.x - sizeX, pos.y - sizeY, pos.z - sizeZ},
            {pos.x + sizeX, pos.y + sizeY, pos.z + sizeZ}
        };
	}

	return AABB{
	    {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
}