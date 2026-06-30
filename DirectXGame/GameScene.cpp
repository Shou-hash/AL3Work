#include "GameScene.h"
#include "Matrix4x4.h"
#include "Player.h"

using namespace KamataEngine;

GameScene::~GameScene() {
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
	delete modelPlayer_;
	delete modelDeathParticles_;

	// 【解放処理】範囲for文（一重）でリスト内の敵を1体ずつ解放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
}

void GameScene::Initialize() {

	mapChipField_ = new MapChipField;
	mapChipField_->LoadMapChipDataFromCSV("Resources/blocks.csv");

	GenerateBlocks();

	model_ = Model::CreateFromOBJ("block", true);

	worldTransform_.Initialize();
	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelPlayer_ = Model::CreateFromOBJ("player", true);
	modelEnemy_ = Model::CreateFromOBJ("player", true);
	modelDeathParticles_ = Model::CreateFromOBJ("particle", true);

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
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(10 + i, 18);

		newEnemy->Initialize(modelEnemy_, &camera_, enemyPosition);
		enemies_.push_back(newEnemy);
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

void GameScene::Update() {
	debugCamera_->Update();

	// プレイヤーの更新処理を呼び出す
	if (player_) {
		player_->Update();
	}

	// 【敵の更新】一重のfor文でリスト内のすべての敵を更新
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	// 【プレイヤーと敵の衝突判定】
	if (player_) {
		player_->CheckEnemyCollision(enemies_);
	}

	// プレイヤーが死亡したときの処理
	if (player_ && player_->IsDead()) {
		if (deathParticles_) {
			if (!deathParticles_->IsInitialized() || deathParticles_->IsFinished()) {
				KamataEngine::Vector3 deathPosition = player_->GetWorldTransform().translation_;
				deathParticles_->Initialize(modelDeathParticles_, &camera_, deathPosition);
			}
		}
	}

	// パーティクルの更新
	if (deathParticles_ && deathParticles_->IsInitialized() && !deathParticles_->IsFinished()) {
		deathParticles_->Update();
	}

	// カメラコントローラーの更新
	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	// ★カメラ行列の更新と転送の修正
	if (isDebugCameraActive_) {
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
		camera_.TransferMatrix(); // ★通常カメラの時も行列をGPUに転送する
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

	skydome->Update();
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

	// プレイヤーの描画
	if (player_) {
		Model::PreDraw();
		player_->Draw();
		Model::PostDraw();
	}

	// 【敵の描画】一重のfor文でリスト内のすべての敵を描画
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			Model::PreDraw();
			enemy->Draw();
			Model::PostDraw();
		}
	}

	// デスパーティクルの描画
	if (deathParticles_ && !deathParticles_->IsFinished()) {
		deathParticles_->Draw();
	}
}