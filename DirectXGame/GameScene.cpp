#include "GameScene.h"
#include "Matrix4x4.h"

using namespace KamataEngine;

GameScene::~GameScene() 
{
	delete model_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
	{
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) 
		{
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	delete debugCamera_;
}

void GameScene::Initialize() 
{
	model_ = Model::Create();

	camera_.Initialize();

	debugCamera_ = new DebugCamera(1280, 720);

	const uint32_t kNumBlockVertical = 10;
	const uint32_t kNumBlockHorizontal = 20;

	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;

	worldTransformBlocks_.resize(kNumBlockVertical);

	for (uint32_t i = 0; i < kNumBlockVertical; i++)
	{
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);

		for (uint32_t j = 0; j < kNumBlockHorizontal; j++) 
		{
			if ((i + j) % 2 == 0) 
			{
				worldTransformBlocks_[i][j] = new WorldTransform();
				worldTransformBlocks_[i][j]->Initialize();

				worldTransformBlocks_[i][j]->translation_.x = kBlockWidth * j;
				worldTransformBlocks_[i][j]->translation_.y = kBlockHeight * i;
			} 
			else
			{
				worldTransformBlocks_[i][j] = nullptr;
			}
		}
	}
}

void GameScene::Update()
{
	debugCamera_->Update();

#ifdef _DEBUG

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) 
	{
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

#endif

	if (isDebugCameraActive_) 
	{
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		
		camera_.TransferMatrix();
	} 
	else 
	{
		camera_.UpdateMatrix();
	}

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
	{
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) 
		{
			if (!worldTransformBlock) { continue; }

			Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

			worldTransformBlock->matWorld_ = affineMatrix;

			worldTransformBlock->TransferMatrix();
		}
	}
}

void GameScene::Draw() 
{
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
	{
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) 
		{
			if (!worldTransformBlock) { continue; }

			Model::PreDraw();

			model_->Draw(*worldTransformBlock, camera_);

			Model::PostDraw();
		}
	}
}