#include "GameScene.h"
#include "Enemy.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "ShieldEnemy.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	delete fade_;
	delete model_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	delete debugCamera_;
	delete modelSkydome_;
	delete mapChipField_;
	delete modelEnemy_;
	delete modelShieldEnemy_;
	delete modelPlayer_;
	delete modelDeathParticles_;

	// 全敵の一括解放（仮想デストラクタにより正しく派生クラスのデストラクタが呼ばれます）
	for (BaseEnemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	delete modelHitEffect_;

	for (HitEffect* effect : hitEffects_) {
		delete effect;
	}
	hitEffects_.clear();
}

void GameScene::Initialize() {
	phase_ = Phase::kFadeIn;
	finished_ = false;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	mapChipField_ = new MapChipField;
	mapChipField_->LoadMapChipDataFromCSV("Resources/blocks.csv");

	GenerateBlocks();

	model_ = Model::CreateFromOBJ("block", true);

	worldTransform_.Initialize();
	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelPlayer_ = Model::CreateFromOBJ("player", true);
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelShieldEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);
	modelDeathParticles_ = Model::CreateFromOBJ("particle", true);
	modelHitEffect_ = Model::CreateFromOBJ("particle", true);

	HitEffect::SetModel(modelHitEffect_);
	HitEffect::SetCamera(&camera_);

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	KamataEngine::Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 18);

	player_ = std::make_unique<Player>();
	player_->Initialize(modelPlayer_, &camera_, playerPosition);
	player_->SetMapChipField(mapChipField_);

	// --- 敵の生成と統合リストへの追加 ---
	// 1. 通常敵の追加
	for (int32_t i = 0; i < 3; i++) {
		Enemy* enemy = new Enemy();
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(10 + i, 18 - i);
		enemy->Initialize(modelEnemy_, &camera_, enemyPosition);
		enemies_.push_back(enemy); // BaseEnemy* 型リストに追加
	}

	// 2. 盾敵の追加
	for (int32_t i = 0; i < 3; i++) {
		ShieldEnemy* enemy = new ShieldEnemy();
		Vector3 shieldEnemyPosition = mapChipField_->GetMapChipPositionByIndex(20, 18 - i);
		enemy->Initialize(modelShieldEnemy_, &camera_, shieldEnemyPosition);
		enemies_.push_back(enemy); // BaseEnemy* 型リストに追加
	}

	deathParticles_ = std::make_unique<DeathParticles>();

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_.get());

	player_->SetCameraController(cameraController_.get());

	Rect stageArea = {10.0f, 90.0f, 5.0f, 100.0f};
	cameraController_->SetMovableArea(stageArea);
	cameraController_->Reset();
}

void GameScene::GenerateBlocks() {
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

// 共通AABB当たり判定関数
bool IsCollision(const Player::AABB& a, const BaseEnemy::AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

void GameScene::Update() {
	ChangePhase();

	switch (phase_) {
	case Phase::kFadeIn:
		UpdateFadeIn();
		break;
	case Phase::kPlay:
		UpdatePlay();
		break;
	case Phase::kDeath:
		UpdateDeath();
		break;
	case Phase::kFadeOut:
		UpdateFadeOut();
		break;
	}

	debugCamera_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
		camera_.TransferMatrix();
	}

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = affineMatrix;
			worldTransformBlock->TransferMatrix();
		}
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kFadeIn:
		if (fade_ && fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:
		if (player_ && player_->IsDead()) {
			phase_ = Phase::kDeath;

			if (deathParticles_) {
				if (!deathParticles_->IsInitialized() || deathParticles_->IsFinished()) {
					KamataEngine::Vector3 deathPosition = player_->GetWorldTransform().translation_;
					deathParticles_->Initialize(modelDeathParticles_, &camera_, deathPosition);
				}
			}
		}
		break;

	case Phase::kDeath:
		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kFadeOut;
			if (fade_) {
				fade_->Start(Fade::Status::FadeOut, 1.0f);
			}
		}
		break;

	case Phase::kFadeOut:
		if (fade_ && fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void GameScene::UpdateFadeIn() {
	if (fade_) {
		fade_->Update();
	}
	skydome->Update();

	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}
}

void GameScene::UpdatePlay() {
	skydome->Update();

	if (player_) {
		player_->Update();
	}

	// 1つのループで全種類の敵を更新（ポリモーフィズム）
	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	// プレイヤー攻撃と敵の判定（1つのループで全敵に対応）
	if (player_) {
		auto attackAABB = player_->GetAttackAABB();
		if (attackAABB.has_value()) {
			for (BaseEnemy* enemy : enemies_) {
				// 死亡・食らい無効化中でない敵と判定
				if (enemy && !enemy->IsDead()) {
					if (IsCollision(attackAABB.value(), enemy->GetAABB())) {

						// 衝突直前の座標を記録（HitEffect表示用）
						Vector3 effectPos = enemy->GetWorldTransform().translation_;

						// 衝突処理（通常敵はOnDead、盾敵はガード等）
						enemy->OnCollision(player_.get());

						// ★ ヒットエフェクト（HitEffect）を生成して追加
						HitEffect* newEffect = new HitEffect();
						newEffect->Initialize(effectPos);
						hitEffects_.push_back(newEffect);
					}
				}
			}
		}
	}

	// ヒットエフェクトの更新と解放
	for (auto it = hitEffects_.begin(); it != hitEffects_.end();) {
		if (*it) {
			(*it)->Update();
			if ((*it)->IsFinished()) {
				delete (*it);
				it = hitEffects_.erase(it);
			} else {
				++it;
			}
		} else {
			it = hitEffects_.erase(it);
		}
	}

	// 全敵のクリーンアップ（死亡アニメーションが完了して IsDead() == true になったものだけ解放）
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		BaseEnemy* enemy = *it;
		if (enemy && enemy->IsDead()) {
			delete enemy;
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}

	if (player_) {
		player_->CheckEnemyCollision(enemies_);
	}
}

void GameScene::UpdateDeath() {
	skydome->Update();

	// 全敵の更新
	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	if (deathParticles_ && deathParticles_->IsInitialized() && !deathParticles_->IsFinished()) {
		deathParticles_->Update();
	}

	for (HitEffect* effect : hitEffects_) {
		if (effect) {
			effect->Update();
		}
	}
}

void GameScene::UpdateFadeOut() {
	if (fade_) {
		fade_->Update();
	}

	skydome->Update();

	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	if (deathParticles_ && deathParticles_->IsInitialized() && !deathParticles_->IsFinished()) {
		deathParticles_->Update();
	}
}

void GameScene::Draw() {
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			Model::PreDraw();
			model_->Draw(*worldTransformBlock, camera_);
			Model::PostDraw();
		}
	}
	skydome->Draw();

	for (HitEffect* effect : hitEffects_) {
		if (effect) {
			Model::PreDraw();
			effect->Draw();
			Model::PostDraw();
		}
	}

	if (player_) {
		Model::PreDraw();
		player_->Draw();
		Model::PostDraw();
	}

	// 全敵の描画（1つのループに統一）
	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			Model::PreDraw();
			enemy->Draw();
			Model::PostDraw();
		}
	}

	if (deathParticles_ && !deathParticles_->IsFinished()) {
		Model::PreDraw();
		deathParticles_->Draw();
		Model::PostDraw();
	}

	if (fade_) {
		Model::PreDraw();
		fade_->Draw();
		Model::PostDraw();
	}
}