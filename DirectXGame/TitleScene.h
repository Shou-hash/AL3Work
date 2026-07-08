#pragma once
#include "Fade.h"

class TitleScene {

public:
	
	~TitleScene();

	void Initialize();
	void Update();
	void Draw();
	

	bool IsFinished() const { return finished_; }

private:

	bool finished_ = false;

	Fade* fade_ = nullptr;
};
