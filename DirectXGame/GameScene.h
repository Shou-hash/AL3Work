#pragma once
#include "Kamataengine.h"
#include <vector>
#include "Matrix4x4.h"

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

	KamataEngine::Model* model_ = nullptr;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	KamataEngine::Camera camera_;

	bool isDebugCameraActive_ = false;

	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};