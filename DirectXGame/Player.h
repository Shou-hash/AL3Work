#pragma once
#include "Kamataengine.h"
#include <3d\WorldTransform.h>

class Player {
public:

	Player();
	~Player();

	void Initialize(KamataEngine::Model* modelPlayer_,KamataEngine::Camera* camera,const KamataEngine::Vector3& position);

private:
	KamataEngine::WorldTransform worldTransform_;
};