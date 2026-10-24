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

namespace {
// スムーズステップ（edge0 以下で 0.0、edge1 以上で 1.0 になめらかに変化する）
float SmoothStep(float edge0, float edge1, float x) {
	float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
} // namespace

FinalBoss::~FinalBoss() {
	// ★ スプライトリソースの解放
	delete spriteParticle_;
	delete spriteTargetMarker_; // ★ 予告マーカースプライト解放
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
	isCollisionDisabled_ = true;

	pendingDown_ = false;
	downWarningTimer_ = 0.0f;
	downTimer_ = 0.0f;
	bodyHitConsumed_ = false;

	state_ = State::kStandby;
	frenzyWaitTimer_ = 0.0f;
	attackTimer_ = 0.0f;
	defeatTimer_ = 0.0f;
	isDefeatAnimationFinished_ = false;
	isDefeatZoomFinished_ = false; // ★ ズームアウト完了フラグのリセット
	particles_.clear();
}

void FinalBoss::StartSpawn() {
	if (state_ == State::kStandby) {
		state_ = State::kSpawn;
		spawnTimer_ = 0.0f;
		particles_.clear();
		worldTransform_.scale_ = {0.0f, 0.0f, 0.0f};
	}
}

void FinalBoss::StartFrenzy() {
	if (state_ == State::kNormal || state_ == State::kSpawn) {
		state_ = State::kFrenzyInit;
		frenzyTimer_ = 0.0f;
	}
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

	float x = worldPos.x * matVP.m[0][0] + worldPos.y * matVP.m[1][0] + worldPos.z * matVP.m[2][0] + matVP.m[3][0];
	float y = worldPos.x * matVP.m[0][1] + worldPos.y * matVP.m[1][1] + worldPos.z * matVP.m[2][1] + matVP.m[3][1];
	float w = worldPos.x * matVP.m[0][3] + worldPos.y * matVP.m[1][3] + worldPos.z * matVP.m[2][3] + matVP.m[3][3];

	if (w <= 0.0f) {
		return {-9999.0f, -9999.0f};
	}

	float ndcX = x / w;
	float ndcY = y / w;

	float screenX = (ndcX + 1.0f) * 0.5f * 1280.0f;
	float screenY = (1.0f - ndcY) * 0.5f * 720.0f;

	return {screenX, screenY};
}

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
	if (state_ == State::kAttack) {
		attackTimer_ += 1.0f / 60.0f;
	}

	if (state_ == State::kSpawn) {
		spawnTimer_ += 1.0f / 60.0f;
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

	if (state_ == State::kFrenzyInit) {
		frenzyTimer_ += 1.0f / 60.0f;
		if (frenzyTimer_ >= 2.0f) {
			state_ = State::kFrenzy;
		}
	}

	if (state_ == State::kFrenzy) {
		frenzyWaitTimer_ += 1.0f / 60.0f;
		if (frenzyWaitTimer_ >= frenzyWaitDuration_ && !isFinalBossDefeated_) {
			StartAttack();
		}
	} else if (state_ == State::kAttack) {
		const float dt = 1.0f / 60.0f;
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
					float marginX = 0.8f;
					float marginY = 0.8f;

					if (meteor.position.x >= playerAABB.min.x - marginX && meteor.position.x <= playerAABB.max.x + marginX && meteor.position.y >= playerAABB.min.y - marginY &&
					    meteor.position.y <= playerAABB.max.y + marginY) {

						player_->OnCollision();
						meteor.active = false;
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

				// ★ 追加: 撃破されていない場合、衝撃演出後にダウン予告へ移行するフラグをセット
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
				crystal22Timer_ = 0.0f;
			}
		}
	}

	// =========================================================
	// ★ 追加: ダウン予告演出 (kDownWarning) ロジック（苦悶・フラフラ揺れてダウンを予兆）
	// =========================================================
	if (state_ == State::kDownWarning) {
		const float dt = 1.0f / 60.0f;
		downWarningTimer_ += dt;

		// 激しく苦しみ・頭を抱えるようにフラフラ揺れるアニメーション
		float progress = downWarningTimer_ / kDownWarningDuration;
		worldTransformBody_.translation_.x = std::sin(downWarningTimer_ * 35.0f) * 0.35f;
		worldTransformBody_.translation_.y = std::cos(downWarningTimer_ * 28.0f) * 0.25f - 0.5f * progress;
		worldTransformBody_.rotation_.z = std::sin(downWarningTimer_ * 20.0f) * 0.25f;
		worldTransformBody_.rotation_.x = 0.4f * SmoothStep(0.0f, 1.0f, progress); // 徐々に前傾姿勢へ

		// 予告パーティクル（衝撃・苦悶の光）を噴出
		if (rand() % 2 == 0) {
			EmitDefeatParticles();
		}

		if (downWarningTimer_ >= kDownWarningDuration) {
			// ダウン予告完了 ➔ ダウン状態 (kDown) へ移行
			state_ = State::kDown;
			downTimer_ = 0.0f;
			downStartPosition_ = worldTransform_.translation_;

			// 伏せる目標座標の設定（下半身はステージ外の奥に置き、上半身をステージ内へ倒し込む）
			KamataEngine::Vector3 targetPos = basePosition_;
			if (player_) {
				targetPos.x = player_->GetWorldTransform().translation_.x;
				targetPos.y = player_->GetWorldTransform().translation_.y - 1.2f;
			} else {
				targetPos.y -= 4.0f;
			}
			targetPos.z = basePosition_.z; // ★ 下半身はステージ外の奥（Z軸）にそのまま配置する
			downTargetPosition_ = targetPos;
		}
	}

	// =========================================================
	// ★ ダウン（伏せ）状態 (kDown) ロジック（伏せる ➔ 本体攻撃受付 ➔ 起き上がる）
	// =========================================================
	if (state_ == State::kDown) {
		const float dt = 1.0f / 60.0f;
		downTimer_ += dt;

		const float fallInDuration = 0.6f;                                        // 倒れ込むのにかかる時間
		const float riseDuration = 0.8f;                                          // 起き上がるのにかかる時間
		const float stayDuration = downDuration_ - fallInDuration - riseDuration; // 伏せている時間

		if (downTimer_ <= fallInDuration) {
			// ① 倒れ込みアニメーション（うつ伏せでステージ手前に倒れ込む）
			float t = std::min(downTimer_ / fallInDuration, 1.0f);
			float easeT = SmoothStep(0.0f, 1.0f, t);

			worldTransform_.translation_ = {
			    downStartPosition_.x + (downTargetPosition_.x - downStartPosition_.x) * easeT, downStartPosition_.y + (downTargetPosition_.y - downStartPosition_.y) * easeT,
			    downStartPosition_.z + (downTargetPosition_.z - downStartPosition_.z) * easeT};

			// 本体をうつ伏せ姿勢（前傾）に傾けて上半身をステージ内へ倒し込む
			worldTransformBody_.rotation_.x = 1.25f * easeT;
			worldTransformBody_.rotation_.z = 0.15f * std::sin(downTimer_ * 10.0f);

		} else if (downTimer_ <= fallInDuration + stayDuration) {
			// ② 伏せ状態を維持（この時間帯のみプレイヤーが本体に攻撃可能）
			worldTransform_.translation_ = downTargetPosition_;
			worldTransformBody_.rotation_.x = 1.25f;
			worldTransformBody_.rotation_.z = 0.05f * std::sin(animTimer_ * 6.0f); // 苦しそうな息遣いの揺れ

		} else if (downTimer_ <= downDuration_) {
			// ③ 起き上がりアニメーション（元の背景位置へ戻る）
			float t = std::min((downTimer_ - fallInDuration - stayDuration) / riseDuration, 1.0f);
			float easeT = SmoothStep(0.0f, 1.0f, t);

			worldTransform_.translation_ = {
			    downTargetPosition_.x + (basePosition_.x - downTargetPosition_.x) * easeT, downTargetPosition_.y + (basePosition_.y - downTargetPosition_.y) * easeT,
			    downTargetPosition_.z + (basePosition_.z - downTargetPosition_.z) * easeT};

			worldTransformBody_.rotation_.x = 1.25f * (1.0f - easeT);
			worldTransformBody_.rotation_.z = 0.0f;

		} else {
			// ④ 起き上がり完了：元の位置に戻り、発狂待機状態へ復帰
			worldTransform_.translation_ = basePosition_;
			worldTransformBody_.rotation_ = {0.0f, 0.0f, 0.0f};
			state_ = isFinalBossDefeated_ ? State::kDefeat : State::kFrenzy;
			frenzyWaitTimer_ = 0.0f;
			downTimer_ = 0.0f;
		}
	}

	// 撃破タイマーおよびスケール更新部 (Update 内)
	if (state_ == State::kDefeat) {
		// ★ カメラのズームアウトが完了してから撃破演出（タイマー進行・スケール・パーティクル）を開始する
		if (isDefeatZoomFinished_) {
			defeatTimer_ += 1.0f / 60.0f;
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
			}
		}
	}

	float floatSpeed = (state_ == State::kFrenzy) ? 3.0f : 1.2f;
	float floatAmp = (state_ == State::kFrenzy) ? 0.8f : 0.4f;
	float floatOffsetY = std::sin(animTimer_ * floatSpeed) * floatAmp;

	// ★ 変更: 伏せ状態(kDown)・予告状態(kDownWarning)以外の時のみ通常通り背景でゆらゆら動く
	if (state_ != State::kDown && state_ != State::kDownWarning && camera_) {
		worldTransform_.translation_.x = (basePosition_.x + drawOffset_.x) + camera_->translation_.x * (1.0f - parallaxFactor_);
		worldTransform_.translation_.y = (basePosition_.y + drawOffset_.y) + camera_->translation_.y * (1.0f - parallaxFactor_) + floatOffsetY;
		worldTransform_.translation_.z = basePosition_.z + drawOffset_.z;
		worldTransform_.translation_.y += std::sin(animTimer_ * 1.2f) * 0.4f;
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (state_ != State::kDown && state_ != State::kDownWarning) {
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
		hitReactionTimer_ += 1.0f / 60.0f;
		worldTransformBody_.rotation_.z = hitReactionRotation_ * (1.0f - std::min(hitReactionTimer_ / 0.45f, 1.0f));
	} else {
		worldTransformBody_.rotation_.z = 0.0f;
		hitReactionRotation_ = 0.0f;

		// ★ 追加: 衝撃アニメーション（のけぞり）終了後、まずダウン予告状態（kDownWarning）へ移行
		if (pendingDown_ && state_ != State::kDefeat) {
			pendingDown_ = false;
			state_ = State::kDownWarning;
			downWarningTimer_ = 0.0f;
			bodyHitConsumed_ = false;
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

	// 崩れ落ち・痙攣アニメーション部 (Update 内)
	if (state_ == State::kDefeat && isDefeatZoomFinished_) {
		const float defeatProgress = std::min(defeatTimer_ / kDefeatDuration, 1.0f);
		const float convulsion = 1.0f - SmoothStep(0.0f, 0.5f, defeatProgress);
		const float fall = SmoothStep(0.25f, 0.85f, defeatProgress);

		// ★ ズームアウト完了後に痙攣（sin/cos振動）と崩れ落ち（fall）のアニメーションが開始する
		worldTransformBody_.translation_.x += std::sin(defeatTimer_ * 55.0f) * 0.45f * convulsion;
		worldTransformBody_.translation_.y += std::cos(defeatTimer_ * 47.0f) * 0.35f * convulsion - 3.0f * fall * fall;
		worldTransformBody_.rotation_.z += std::sin(defeatTimer_ * 38.0f) * 0.12f * convulsion + 0.9f * fall;

		const float bulge = 1.0f + 0.15f * std::sin(std::min(defeatProgress / 0.3f, 1.0f) * std::numbers::pi_v<float>);
		worldTransformBody_.scale_ = {bulge, bulge, bulge};
	}

	KamataEngine::Matrix4x4 localMatBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatBody, worldTransform_.matWorld_);
	worldTransformBody_.TransferMatrix();

	float handSpeed = (state_ == State::kAttack) ? 6.0f : ((state_ == State::kFrenzy) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f));
	float handSpread = (state_ == State::kAttack) ? (std::min(attackTimer_ / kAttackIntroDuration, 1.0f) * 2.4f + std::sin(animTimer_ * 4.0f) * 0.2f)
	                                              : ((state_ == State::kFrenzyInit) ? (std::min(frenzyTimer_ / 2.0f, 1.0f) * 1.8f) : ((state_ == State::kFrenzy) ? 1.5f : 0.0f));

	if (state_ == State::kDefeat) {
		const float handFall = SmoothStep(0.25f, 0.85f, std::min(defeatTimer_ / kDefeatDuration, 1.0f));
		handSpread = 1.5f * (1.0f - handFall);
		handSpeed = 6.0f * (1.0f - handFall) + 0.8f;
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

	float baseCrystalSpeed = (state_ == State::kAttack) ? 6.0f : ((state_ == State::kFrenzy) ? 4.5f : ((state_ == State::kFrenzyInit) ? 7.0f : 2.0f));
	float crystalAmp = (state_ == State::kAttack) ? 1.0f : ((state_ == State::kFrenzy) ? 1.2f : 0.6f);

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

void FinalBoss::DrawTargetMarkers() {
	if (!spriteTargetMarker_ || !camera_ || state_ != State::kAttack) {
		return;
	}

	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();
	if (dxCommon) {
		KamataEngine::Sprite::PreDraw(dxCommon->GetCommandList(), KamataEngine::Sprite::BlendMode::kAdd);
	} else {
		KamataEngine::Sprite::PreDraw();
	}

	if (meteorShowerStarted_) {
		for (size_t i = 0; i < kNumMeteorShower; ++i) {
			const auto& meteor = meteorCrystals_[i];
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
		spriteHpBar_->Draw();
		KamataEngine::Sprite::PostDraw();
	}

	DrawParticles();
}

void FinalBoss::OnCollision(Player* player) {
	(void)player;
	// ★ 追加: 伏せ状態(kDown)の時のみプレイヤーの本体攻撃を受理しダメージを与える（1回のダウンで1回のみ）
	if (state_ == State::kDown && !bodyHitConsumed_ && !isFinalBossDefeated_) {
		finalBossHp_ = std::max(0, finalBossHp_ - 1);
		finalBossHitEvent_ = true;
		bodyHitConsumed_ = true; // 多重ヒット防止
		hitReactionTimer_ = 0.0f;
		hitReactionRotation_ = 0.25f;

		// ★ 追加: 被弾後はすぐに起き上がりアニメーションへ移行（1回のダウンで喰らえるダメージ量を1回分に制限）
		const float fallInDuration = 0.6f;
		const float riseDuration = 0.8f;
		const float stayDuration = downDuration_ - fallInDuration - riseDuration;
		if (downTimer_ < fallInDuration + stayDuration) {
			downTimer_ = fallInDuration + stayDuration;
		}

		if (finalBossHp_ <= 0) {
			isFinalBossDefeated_ = true;
			state_ = State::kDefeat;
			defeatTimer_ = 0.0f;
		}
	}
}

void FinalBoss::OnDead() { isDead_ = true; }

BaseEnemy::AABB FinalBoss::GetAABB() const {
	// ★ 追加: 伏せ状態 (kDown) の時、プレイヤー攻撃用の拡張されたAABBを返す（Y/Z軸の判定を大幅に拡大）
	if (state_ == State::kDown) {
		KamataEngine::Vector3 pos = {worldTransformBody_.matWorld_.m[3][0], worldTransformBody_.matWorld_.m[3][1], worldTransformBody_.matWorld_.m[3][2]};
		float sizeX = 5.0f;  // ★ X軸の判定範囲拡大
		float sizeY = 3.0f;  // ★ Y軸の判定範囲を大幅に拡大
		float sizeZ = 5.0f; // ★ Z軸の判定範囲を大幅に拡大（プレイヤーの位置とZ座標がずれていても確実にヒット）
		return AABB{
		    {pos.x - sizeX, pos.y - sizeY, pos.z - sizeZ},
            {pos.x + sizeX, pos.y + sizeY, pos.z + sizeZ}
        };
	}

	// 通常時は奥行き（背景）にいるため空の判定を返す
	return AABB{
	    {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
}