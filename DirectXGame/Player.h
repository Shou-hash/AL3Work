#pragma once
#include "Kamataengine.h"

//<summary>
// プレイヤークラス
///</summary>
class Player 
{
public:
	// デストラクタ
	~Player();

	// 初期化
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);

	// 更新
	void Update();

	// 描画
	void Draw(KamataEngine::Camera* camera);

private:

	KamataEngine::WorldTransform worldTransform_;
	
	KamataEngine::Model* model_ = nullptr;
	
	uint32_t textureHandle_ = 0u;

};