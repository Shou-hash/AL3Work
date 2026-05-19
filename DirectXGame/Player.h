#pragma once
#include "Kamataengine.h"
#include <3d\WorldTransform.h>

class Player {
public:

	Player();
	~Player();

	void Initialize(KamataEngine::Model* modelPlayer_,KamataEngine::Camera* camera,const KamataEngine::Vector3& position);

	void Update();

	void Draw();

	static Player* player_;

	KamataEngine::Vector3 velocity_ = {};

	static inline const float kAcceleration = 0.5f;

private:

	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Model* modelPlayer_ = nullptr;

	KamataEngine::WorldTransform worldTransform_;
};