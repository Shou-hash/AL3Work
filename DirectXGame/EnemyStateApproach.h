#pragma once
#include "BaseEnemyState.h"

// 接近フェーズ
class EnemyStateApproach : public BaseEnemyState {
public:
	// コンストラクタ
	EnemyStateApproach(Enemy* enemy);

	// 更新処理
	void Update() override;
};