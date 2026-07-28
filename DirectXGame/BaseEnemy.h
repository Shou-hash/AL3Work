#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>

class Player;

/// <summary>
/// 敵基底クラス
/// </summary>
class BaseEnemy {
public:
	// 仮想デストラクタ
	virtual ~BaseEnemy() = default;

	// 初期化
	virtual void Initialize();

	// 更新
	virtual void Update();

	// 描画
	virtual void Draw();

	// 当たり判定用 AABB 構造体
	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	// 共通ゲッターおよび仮想関数
	virtual const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	virtual bool IsDead() const { return isDead_; }
	virtual void OnDead();
	virtual AABB GetAABB() const;
	virtual void OnCollision(Player* player);

protected:
	KamataEngine::WorldTransform worldTransform_;
	bool isDead_ = false;
	bool isCollisionDisabled_ = false;
};