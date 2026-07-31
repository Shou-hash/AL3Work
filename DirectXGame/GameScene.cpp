#include "GameScene.h"

using namespace KamataEngine;

GameScene::~GameScene() 
{
	delete model_;
	delete player_;

}

void GameScene::Initialize()
{
	// カメラの初期化
	camera_.Initialize();

	// 背景などの初期化
	textureHandle_ = TextureManager::Load("uvChecker.png");
	model_ = Model::Create();

	// プレイヤーの生成と初期化
	player_ = new Player();
	player_->Initialize(model_, textureHandle_);
}

void GameScene::Update()
{
	// カメラの行列を更新
	camera_.UpdateMatrix();

	player_->Update();
}

void GameScene::Draw() 
{
	player_->Draw(&camera_);
}