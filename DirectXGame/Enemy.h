#pragma once
#include "BaseEnemy.h"

class Player;
class MapChipField; // ★追加：MapChipFieldの不完全宣言

class Enemy final : public BaseEnemy {
public:
	enum class Behavior {
		kRoot,
		kDead,
	};

	// ★変更：MapChipFieldのポインタを追加
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField);

	void Initialize() override {}
	void Update() override;
	void Draw() override;
	void OnDead() override;
	void OnCollision(Player* player) override;
	AABB GetAABB() const override;

	void BehaviorRootUpdate();
	void BehaviorDeadUpdate();

	// 調整項目の登録・反映用の static 関数
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	MapChipField* mapChipField_ = nullptr; // ★追加：メンバー変数の保持

	KamataEngine::Vector3 velocity_ = {0, 0, 0};

	static inline float kWalkspeed = 0.05f;
	static inline float kWalkMotionAnglestart = -20.0f;
	static inline float kWalkMotionAngleEnd = 30.0f;
	static inline float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f;

	Behavior behavior_ = Behavior::kRoot;
	float deadTimer_ = 0.0f;
	static inline float kDeadDuration = 1.0f;
};