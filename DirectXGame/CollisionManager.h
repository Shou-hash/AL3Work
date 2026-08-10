#pragma once
#include "Collider.h"
#include <list>

/// <summary>
/// 衝突判定マネージョ
/// </summary>
class CollisionManager {
public:
	/// <summary>
	/// コライダーの追加
	/// </summary>
	void AddCollider(Collider* collider) { colliders_.push_back(collider); }

	/// <summary>
	/// コライダーリストのクリア
	/// </summary>
	void ClearColliders() { colliders_.clear(); }

	/// <summary>
	/// 全ての組み合わせの当たり判定チェック
	/// </summary>
	void CheckAllCollisions();

private:
	/// <summary>
	/// 2つのコライダー間の衝突判定と通知
	/// </summary>
	void CheckCollisionPair(Collider* colliderA, Collider* colliderB);

private:
	// コライダーリスト
	std::list<Collider*> colliders_;
};