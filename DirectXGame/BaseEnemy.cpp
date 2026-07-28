#include "BaseEnemy.h"

void BaseEnemy::Initialize() {}

void BaseEnemy::Update() {}

void BaseEnemy::Draw() {}

void BaseEnemy::OnDead() { isDead_ = true; }

void BaseEnemy::OnCollision(Player* player) { (void)player; }

BaseEnemy::AABB BaseEnemy::GetAABB() const {
	if (isCollisionDisabled_) {
		return AABB{
		    {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f}
        };
	}

	AABB aabb;
	const auto& pos = worldTransform_.translation_;
	aabb.min = {pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f};
	aabb.max = {pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f};
	return aabb;
}