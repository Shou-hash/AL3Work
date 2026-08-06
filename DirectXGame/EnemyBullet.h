#pragma once
#include "3d/Model.h"
#include "3d/WorldTransform.h"
#include "Collider.h"
#include "KamataEngine.h"

class Player;

/// <summary>
/// 敵の弾（Colliderを継承）
/// </summary>
class EnemyBullet : public Collider {
public:
	static const int32_t kLifeTime = 60 * 5;

	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void Update();
	void Draw(const KamataEngine::Camera& camera);

	bool IsDead() const { return isDead_; }

	void SetPlayer(Player* player) { player_ = player; }

	// ★ Colliderの関数をオーバーライド
	void OnCollision() override;
	KamataEngine::Vector3 GetWorldPosition() const override;

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	KamataEngine::Vector3 velocity_;
	int32_t deathTimer_ = kLifeTime;
	bool isDead_ = false;

	Player* player_ = nullptr;
};