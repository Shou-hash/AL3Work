#pragma once
#include "KamataEngine.h"
#include <vector>

class PlayerHp {
public:
	void Initialize(KamataEngine::Model* model, int32_t maxHp);
	void Update(const KamataEngine::Vector3& cameraPos);
	void Draw(const KamataEngine::Camera& camera);

	void DecreaseHp() {
		if (currentHp_ > 0)
			currentHp_--;
	}
	void IncreaseHp() {
		if (currentHp_ < maxHp_)
			currentHp_++;
	}
	int32_t GetCurrentHp() const { return currentHp_; }
	bool IsDead() const { return currentHp_ <= 0; }

private:
	KamataEngine::Model* model_ = nullptr;
	int32_t maxHp_ = 4;
	int32_t currentHp_ = 4;
	std::vector<KamataEngine::WorldTransform*> worldTransforms_;
};