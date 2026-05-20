#pragma once
#include "Kamataengine.h"

class CameraController {
public:

	void Initialize();

	void Update();


private:

	KamataEngine::Camera* camera_ = nullptr;

};
