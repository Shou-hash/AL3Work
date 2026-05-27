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

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	KamataEngine::Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 17);

	// プレイヤーの生成と初期化
	player_ = std::make_unique<Player>();
	player_->Initialize(modelPlayer_, &camera_, playerPosition);

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_.get());

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
		player_->Draw();
	}
}