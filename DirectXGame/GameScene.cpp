#include "GameScene.h"
#include "Matrix4x4.h"
#include "Player.h"

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

	// 範囲for文（一重）でリスト内の敵を1体ずつ解放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		delete shieldEnemy;
	}
	enemies_.clear();

	delete modelHitEffect_;

	// ヒットエフェクトリストの解放
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

	// プレイヤーの生成と初期化
	player_ = std::make_unique<Player>();
	player_->Initialize(modelPlayer_, &camera_, playerPosition);

	player_->SetMapChipField(mapChipField_);

	// 【敵の生成】スライドの指示通り new Enemy() で生成し、1体ずつ異なる座標をセットしてリストに追加
	for (int32_t i = 0; i < 3; i++) {
		// インデックス(10, 18)を基準に、1体ごとにX軸方向にずらして異なる座標を作成
		Enemy* newEnemy = new Enemy();
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(10 + i, 18 - i);

		newEnemy->Initialize(modelEnemy_, &camera_, enemyPosition);
		enemies_.push_back(newEnemy);
	}

	for (int32_t i = 0; i < 3; i++) {
		// インデックス(10, 18)を基準に、1体ごとにX軸方向にずらして異なる座標を作成
		ShieldEnemy* newShieldEnemy = new ShieldEnemy();
		Vector3 shieldEnemyPosition = mapChipField_->GetMapChipPositionByIndex(20, 18 - i);

		newShieldEnemy->Initialize(modelShieldEnemy_, &camera_, shieldEnemyPosition);
		shieldEnemies_.push_back(newShieldEnemy);
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

bool IsCollision(const Player::AABB& a, const Enemy::AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// ShieldEnemy用の IsCollision オーバーロード
bool IsCollision(const Player::AABB& a, const ShieldEnemy::AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

void GameScene::Update() {

	// フェーズの切り替え判定
	ChangePhase();

	// フェーズごとの更新処理
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

	// 両方のフェーズで共通して行う処理（デバッグカメラや行列更新など）
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

//　フェーズの切り替え処理
void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kFadeIn:
		// フェードインが終了したらプレイフェーズへ
		if (fade_ && fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:
		// プレイヤーが死亡フラグを持っていたらデス演出フェーズへ移行
		if (player_ && player_->IsDead()) {
			phase_ = Phase::kDeath;

			// デス演出フェーズに入った瞬間（トリガー）でパーティクルを発生させる
			if (deathParticles_) {
				if (!deathParticles_->IsInitialized() || deathParticles_->IsFinished()) {
					KamataEngine::Vector3 deathPosition = player_->GetWorldTransform().translation_;
					deathParticles_->Initialize(modelDeathParticles_, &camera_, deathPosition);
				}
			}
		}
		break;

	case Phase::kDeath:
		// デス演出が終了したらフェードアウトを開始し、フェードアウトフェーズへ
		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kFadeOut;
			if (fade_) {
				fade_->Start(Fade::Status::FadeOut, 1.0f);
			}
		}
		break;

	case Phase::kFadeOut:
		// フェードアウトが終了したらシーン終了フラグを立てる
		if (fade_ && fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

// フェードイン処理
void GameScene::UpdateFadeIn() {
	if (fade_) {
		fade_->Update();
	}

	// 背景などの描画物は動かすために更新
	skydome->Update();

	// カメラコントローラーの更新
	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}
}

// ゲームプレイフェーズの更新
void GameScene::UpdatePlay() {
	// 天球の更新
	skydome->Update();

	// 自キャラの更新
	if (player_) {
		player_->Update();
	}

	// 敵の更新（複数）
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	// 敵の更新（盾敵）
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		if (shieldEnemy) {
			shieldEnemy->Update();
		}
	}

	// プレイヤー攻撃と敵の当たり判定
	if (player_) {
		auto attackAABB = player_->GetAttackAABB();
		if (attackAABB.has_value()) {
			// 通常敵との判定
			for (Enemy* enemy : enemies_) {
				if (enemy && !enemy->IsDead() && IsCollision(attackAABB.value(), enemy->GetAABB())) {
					HitEffect* newEffect = new HitEffect();
					newEffect->Initialize(enemy->GetWorldTransform().translation_);
					hitEffects_.push_back(newEffect);
					enemy->OnDead();
				}
			}

			// 盾敵との判定 (追加)
			for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
				if (shieldEnemy && !shieldEnemy->IsDead() && IsCollision(attackAABB.value(), shieldEnemy->GetAABB())) {
					// ShieldEnemy 内部で正面判定・ガードエフェクト生成・または OnDead() を行う
					shieldEnemy->OnCollision(player_.get());
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

	// 通常敵のクリーンアップ
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* enemy = *it;
		if (enemy && enemy->IsDead()) {
			delete enemy;
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	// 盾敵のクリーンアップ (追加)
	for (auto it = shieldEnemies_.begin(); it != shieldEnemies_.end();) {
		ShieldEnemy* shieldEnemy = *it;
		if (shieldEnemy && shieldEnemy->IsDead()) {
			delete shieldEnemy;
			it = shieldEnemies_.erase(it);
		} else {
			++it;
		}
	}
	
	// カメラコントローラーの更新
	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}

	// 全ての当たり判定（自キャラと敵の衝突判定など）
	if (player_) {
		player_->CheckEnemyCollision(enemies_);
	}
}

// デス演出フェーズの更新
void GameScene::UpdateDeath() {
	// 天球の更新
	skydome->Update();

	// 敵の更新（複数）
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	// 盾敵の更新
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		if (shieldEnemy)
			shieldEnemy->Update();
	}

	// デスパーティクルの更新（このフェーズでのみ行う）
	if (deathParticles_ && deathParticles_->IsInitialized() && !deathParticles_->IsFinished()) {
		deathParticles_->Update();
	}

	// 自キャラの更新とカメラコントローラーの更新を省くことで、
	// 死亡時にカメラが勝手に動き回ったりプレイヤーが操作できてしまうのを防ぎます。

	// ヒットエフェクトの更新
	for (HitEffect* effect : hitEffects_) {
		if (effect) {
			effect->Update();
		}
	}
}

// フェードアウト処理
void GameScene::UpdateFadeOut() {
	if (fade_) {
		fade_->Update();
	}

	// フェードアウト中も背景やパーティクルの最後の余韻を描画・更新し続ける
	skydome->Update();

	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	if (deathParticles_ && deathParticles_->IsInitialized() && !deathParticles_->IsFinished()) {
		deathParticles_->Update();
	}
}

void GameScene::Draw() {
	// ブロック描画
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

	// ヒットエフェクトの描画
	for (HitEffect* effect : hitEffects_) {
		if (effect) {
			Model::PreDraw();
			effect->Draw();
			Model::PostDraw();
		}
	}

	// プレイヤーの描画
	if (player_) {
		Model::PreDraw();
		player_->Draw();
		Model::PostDraw();
	}

	// 敵の描画 一重のfor文でリスト内のすべての敵を描画
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			Model::PreDraw();
			enemy->Draw();
			Model::PostDraw();
		}
	}

	// 盾敵の描画 (追加)
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		if (shieldEnemy) {
			Model::PreDraw();
			shieldEnemy->Draw();
			Model::PostDraw();
		}
	}

	// デスパーティクルの描画
	if (deathParticles_ && !deathParticles_->IsFinished()) {
		Model::PreDraw();
		deathParticles_->Draw();
		Model::PostDraw();
	}

	// 最前面にフェードのスプレイトを描画
	if (fade_) {
		Model::PreDraw();
		fade_->Draw();
		Model::PostDraw();
	}

	
}