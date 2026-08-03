#pragma once
#include "BaseEnemyState.h"

// 離脱フェーズ
class EnemyStateLeave : public BaseEnemyState {
public:
	EnemyStateLeave(Enemy* enemy);
	void Update() override;
};