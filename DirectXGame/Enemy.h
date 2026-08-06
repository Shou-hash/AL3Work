#pragma once
#include "3d/AxisIndicator.h"
#include "3d/DebugCamera.h"
#include "EnemyBullet.h"
#include "TimedCall.h"
#include <KamataEngine.h>
#include <functional>
#include <list>

// 自機クラスの前方宣言
class Player;
class BaseEnemyState;

class Enemy {
public:
	static const uint32_t kFireInterval = 60;

	~Enemy();

	// Initializeで player も受け取れるように変更
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, Player* player);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	void ChangeState(BaseEnemyState* newState);

	void InitializeApproachPhase();
	void FireAndReset();
	void ClearTimedCalls();
	void Fire();

	// Player の Setter
	void SetPlayer(Player* player) { player_ = player; }

	// 敵自身のワールド座標を取得する関数
	KamataEngine::Vector3 GetWorldPosition();

	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	BaseEnemyState* state_ = nullptr;

	// 自機キャラのポインタ（所有権は持たない）
	Player* player_ = nullptr;

	std::list<EnemyBullet*> bullets_;
	std::list<TimedCall*> timedCalls_;
};