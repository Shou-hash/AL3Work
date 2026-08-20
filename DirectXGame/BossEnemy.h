#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>

class MapChipField;
class Player;

enum class BossEnemyLRDirection {
	kLeft,
	kRight,
};

class BossEnemy : public BaseEnemy {
public:
	BossEnemy() = default;
	~BossEnemy() override;

	void Initialize(
	    KamataEngine::Model* modelBody, KamataEngine::Model* modelHead, KamataEngine::Model* modelLeft, KamataEngine::Model* modelRight, KamataEngine::Camera* camera,
	    const KamataEngine::Vector3& position, MapChipField* mapChipField);

	void Update() override;
	void Draw() override;
	void OnCollision(Player* player) override;
	void OnDead() override;
	AABB GetAABB() const override;

	int32_t GetHp() const { return hp_; }

private:
	KamataEngine::Matrix4x4 MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b);

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	// 4部位のモデル
	KamataEngine::Model* modelBody_ = nullptr;
	KamataEngine::Model* modelHead_ = nullptr;
	KamataEngine::Model* modelLeft_ = nullptr;
	KamataEngine::Model* modelRight_ = nullptr;

	// 4部位のWorldTransform
	KamataEngine::WorldTransform worldTransformBody_;
	KamataEngine::WorldTransform worldTransformHead_;
	KamataEngine::WorldTransform worldTransformLeft_;
	KamataEngine::WorldTransform worldTransformRight_;

	float animTimer_ = 0.0f;

	// ボス用HPおよびクールダウン時間
	int32_t hp_ = 5;
	int32_t maxHp_ = 5;
	float damageCooldown_ = 0.0f;

	// 歩行移動関連
	BossEnemyLRDirection lrDirection_ = BossEnemyLRDirection::kLeft;
	static inline const float kWalkspeed = 0.03f;

	// HPバー表示用スプライト
	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* spriteHpBG_ = nullptr;
	KamataEngine::Sprite* spriteHpBar_ = nullptr;
};