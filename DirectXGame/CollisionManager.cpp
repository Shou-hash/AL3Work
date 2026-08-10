#include "CollisionManager.h"
#include <cmath>

void CollisionManager::CheckAllCollisions() {
	// リスト内の全コライダーの総当たり判定
	std::list<Collider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		Collider* colliderA = *itrA;

		// 重複判定を避けるため、itrA の次の要素からループを回す
		std::list<Collider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			Collider* colliderB = *itrB;

			// ペアの判定処理へ
			CheckCollisionPair(colliderA, colliderB);
		}
	}
}

void CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {
	// 衝突フィルタリングによる判定スキップ
	if ((colliderA->GetCollisionAttribute() & colliderB->GetCollisionMask()) == 0 || (colliderB->GetCollisionAttribute() & colliderA->GetCollisionMask()) == 0) {
		return;
	}

	// 各ワールド座標の取得
	KamataEngine::Vector3 posA = colliderA->GetWorldPosition();
	KamataEngine::Vector3 posB = colliderB->GetWorldPosition();

	// 2点間の距離の2乗を計算
	float dx = posB.x - posA.x;
	float dy = posB.y - posA.y;
	float dz = posB.z - posA.z;
	float distSquare = dx * dx + dy * dy + dz * dz;

	// 半径の和の2乗
	float radiusSum = colliderA->GetRadius() + colliderB->GetRadius();
	float radiusSumSquare = radiusSum * radiusSum;

	// 球同士の交差判定
	if (distSquare <= radiusSumSquare) {
		// 衝突コールバックの呼び出し
		colliderA->OnCollision();
		colliderB->OnCollision();
	}
}