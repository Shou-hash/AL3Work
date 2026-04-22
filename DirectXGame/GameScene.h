#pragma once
#include "Kamataengine.h"

class GameScene 
{
public:

	~GameScene();

	//初期化
	void Initialize();

	//更新
	void Update();

	// 描画
	void Draw();

	
private:
	uint32_t textureHandle_ = 0;

	uint32_t modelHandle_ = 0;
	
	KamataEngine::Sprite* sprite_ = nullptr;

	KamataEngine::Model* model_ = nullptr;

	//ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// カメラ
	KamataEngine::Camera camera_;

	//サウンドデータハンドル
	uint32_t soundDataHandle_ = 0;

	//音声再生ハンドル
	uint32_t voiceHandle_ = 0;

	// imgui用の変数
	float inputFloat[3] = {0.0f, 0.0f, 0.0f};

	//デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};