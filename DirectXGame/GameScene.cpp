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

	// 背景などの初期化（既存コード）
	textureHandle_ = TextureManager::Load("uvChecker.png");
	model_ = Model::Create();
	worldTransform_.Initialize();

	// プレイヤーの生成と初期化
	player_ = new Player();
	player_->Initialize(model_, textureHandle_);
}

void GameScene::Update() 
{
	player_->Update(); 
}

void GameScene::Draw() 
{
	player_->Draw(&camera_);
}