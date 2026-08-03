#include "EnemyStateApproach.h"
#include "Enemy.h"
#include "EnemyStateLeave.h"

EnemyStateApproach::EnemyStateApproach(Enemy* enemy) : BaseEnemyState("State Approach", enemy) {}

void EnemyStateApproach::Update() {
	DebugLog(); // ログ表示

	if (enemy_ == nullptr) {
		return;
	}

	// Enemy から WorldTransform を取得
	KamataEngine::WorldTransform& transform = enemy_->GetWorldTransform();

	// 接近（手前に進む）
	KamataEngine::Vector3 approachVelocity = {0.0f, 0.0f, -0.2f};
	transform.translation_.x += approachVelocity.x;
	transform.translation_.y += approachVelocity.y;
	transform.translation_.z += approachVelocity.z;

	// z < 0.0f に達したら「離脱ステート」へ遷移！
	if (transform.translation_.z < 0.0f) {
		enemy_->ChangeState(new EnemyStateLeave(enemy_));
	}
}