#pragma once
#include "3d/DebugCamera.h"
#include "Collider.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "Player.h"
#include "CollisionManager.h"

class GameScene {
public:
	~GameScene();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	/// <summary>
	/// 衝突判定と応答
	/// </summary>
	void CheckAllCollisions();

private:
	/// <summary>
	/// コライダーペア間の衝突判定と応答
	/// </summary>
	void CheckCollisionPair(Collider* colliderA, Collider* colliderB);

private:
	Enemy* enemy_ = nullptr;
	KamataEngine::Model* enemyModel_ = nullptr;
	uint32_t enemyTextureHandle_ = 0;

	KamataEngine::Camera camera_;

	Player* player_ = nullptr;
	uint32_t playerTex_ = 0;
	KamataEngine::Model* model_ = nullptr;

	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	KamataEngine::Input* input_ = nullptr;

	// 衝突マネージャ
	CollisionManager* collisionManager_ = nullptr;
};