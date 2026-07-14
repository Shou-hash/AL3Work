#include "TitleScene.h"
#include "Kamataengine.h"

TitleScene::~TitleScene(){ delete fade_; }

void TitleScene::Initialize() {
	finished_ = false; // シーン再開時にフラグをリセット
	phase_ = Phase::FadeIn; // 初期フェーズを設定

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void TitleScene::Update() {
	fade_->Update();

	switch (phase_) {
	case Phase::FadeIn:
		// フェードインが終了したら通常状態へ
		if (fade_->IsFinished()) {
			phase_ = Phase::Normal;
		}
		break;

	case Phase::Normal:
		// スペースキーが押されたらフェードアウトを開始
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::FadeOut;
		}
		break;

	case Phase::FadeOut:
		// フェードアウトが終了したらシーン終了フラグを立てる
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void TitleScene::Draw() {
	// 必要に応じてタイトルの描画処理を記述
	fade_->Draw();
}