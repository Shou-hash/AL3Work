#pragma once
#include <KamataEngine.h>

class Fade {

public:

	void Initialize();
	void Update();
	void Draw();

private:

	KamataEngine::Sprite* sprite_ = nullptr;

};
