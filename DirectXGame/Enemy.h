#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>

class Enemy {
public:
	// プレイヤーを参考に、モデル・カメラ・初期位置を受け取る初期化関数
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update();
	void Draw();

private:
	// 描画に必要なエンジン系のポインタ
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;

	// 位置・回転・スケールを管理するワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
};