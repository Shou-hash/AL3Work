#include "TitleScene.h"
#include "Kamataengine.h"

void TitleScene::Initialize() {
	finished_ = false; // シーン再開時にフラグをリセット
}

void TitleScene::Update() {
	if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
		finished_ = true;
	}
}

void TitleScene::Draw() {
	// 必要に応じてタイトルの描画処理を記述
}