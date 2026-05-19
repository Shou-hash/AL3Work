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

	delete Player::player_;
}

void GameScene::Initialize() {

	mapChipField_ = new MapChipField;
	mapChipField_->LoadMapChipDataFromCSV("Resources/blocks.csv");

	GenerateBlocks();

	model_ = Model::Create();

	worldTransform_.Initialize();

	camera_.Initialize();

	debugCamera_ = new DebugCamera(1280, 720);

	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	modelPlayer_ = Model::CreateFromOBJ("player", true);

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	KamataEngine::Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2,19);

	Player::player_->Initialize(modelPlayer_, &camera_, playerPosition);
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

	worldTransform_.translation_.x += Player::player_->velocity_.x;
	worldTransform_.translation_.y += Player::player_->velocity_.y;
	worldTransform_.translation_.z += Player::player_->velocity_.z;

	if (Input::GetInstance()->PushKey(DIK_RIGHT) ||
		Input::GetInstance()->PushKey(DIK_LEFT)) 
	{
		KamataEngine::Vector3 acceleration = {};
		if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
			acceleration.x += Player::kAcceleration;

			if (Input::GetInstance()->PushKey(DIK_RIGHT)) 
			{
				acceleration.x += Player::kAcceleration;
			} 
			else if (Input::GetInstance()->PushKey(DIK_LEFT)) 
			{
				acceleration.x -= Player::kAcceleration;
			}
			Player::player_->velocity_.x += acceleration.x;
			Player::player_->velocity_.y += acceleration.y;
			Player::player_->velocity_.z += acceleration.z;
		}

	}

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

	Player::player_->Draw();

}