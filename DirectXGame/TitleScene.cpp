#include "TitleScene.h"
#include "Kamataengine.h"

TitleScene::~TitleScene(){ delete fade_; }

void TitleScene::Initialize() {
	finished_ = false; // シーン再開時にフラグをリセット

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void TitleScene::Update() {
	fade_->Update();
	if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
		finished_ = true;
	}
}

void TitleScene::Draw() {
	// 必要に応じてタイトルの描画処理を記述
	fade_->Draw();
}