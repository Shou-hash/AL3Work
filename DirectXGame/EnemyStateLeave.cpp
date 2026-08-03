#include "EnemyStateLeave.h"
#include "Enemy.h"

EnemyStateLeave::EnemyStateLeave(Enemy* enemy) : BaseEnemyState("State Leave", enemy) {}

void EnemyStateLeave::Update() {
	DebugLog(); // ログ表示

	if (enemy_ == nullptr) {
		return;
	}

	// Enemy から WorldTransform を取得
	KamataEngine::WorldTransform& transform = enemy_->GetWorldTransform();

	// 離脱（斜め上奥へ進む）
	KamataEngine::Vector3 leaveVelocity = {-0.1f, 0.1f, -0.2f};
	transform.translation_.x += leaveVelocity.x;
	transform.translation_.y += leaveVelocity.y;
	transform.translation_.z += leaveVelocity.z;
}