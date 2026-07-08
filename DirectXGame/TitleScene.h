#pragma once
class TitleScene {
public:
	
	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:

	bool finished_ = false;
};
