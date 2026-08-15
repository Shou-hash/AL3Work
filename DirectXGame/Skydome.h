#pragma once
#include "Kamataengine.h"

class Skydome {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);

	void Update();

	void Draw();

	void SetModel(KamataEngine::Model* model) { model_ = model; }

private:
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
};