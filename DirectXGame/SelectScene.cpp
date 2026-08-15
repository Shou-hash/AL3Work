#include "SelectScene.h"
#include "Matrix4x4.h"
#include "StageManager.h"
#ifdef _DEBUG
#include <imgui.h>
#endif

SelectScene::~SelectScene() {
	delete fade_;
	delete modelSkydome_;
}

void SelectScene::Initialize(StageManager* stageDataManager) {
	stageManager_ = stageDataManager;
	finished_ = false;
	phase_ = Phase::FadeIn;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	camera_.Initialize();
	camera_.translation_ = {10.0f, 5.0f, -15.0f};

	modelSkydome_ = KamataEngine::Model::CreateFromOBJ("skydome", true);
	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	if (stageManager_) {
		currentSelectIndex_ = stageManager_->GetCurrentStageIndex();
	}
}

void SelectScene::Update() {
	fade_->Update();
	skydome->Update();

	camera_.UpdateMatrix();

	// シーンフェーズ管理
	switch (phase_) {
	case Phase::FadeIn:
		if (fade_->IsFinished())
			phase_ = Phase::Normal;
		break;
	case Phase::Normal:
		// 左右キーでステージ選択を変更
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_LEFT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_A)) {
			currentSelectIndex_--;
			if (currentSelectIndex_ < 0) {
				currentSelectIndex_ = 2; // ステージ3 (インデックス2) へループ
			}
		}
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_RIGHT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_D)) {
			currentSelectIndex_++;
			if (currentSelectIndex_ > 2) {
				currentSelectIndex_ = 0; // ステージ1 (インデックス0) へループ
			}
		}

		// 決定キー（SPACE）でステージ確定＆フェードアウト開始
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			if (stageManager_) {
				stageManager_->SetCurrentStageIndex(currentSelectIndex_);
			}
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::FadeOut;
		}
		break;
	case Phase::FadeOut:
		if (fade_->IsFinished())
			finished_ = true;
		break;
	}

#ifdef _DEBUG
	ImGui::Begin("Select Scene");
	ImGui::Text("Select Stage with Left/Right arrow keys and press SPACE to start.");
	if (stageManager_) {
		int stageCount = stageManager_->GetStageCount();
		if (ImGui::SliderInt("Select Stage Index", &currentSelectIndex_, 0, stageCount - 1)) {
			// スライダー操作への追従
		}
		ImGui::Text("Current Selection: %s", stageManager_->GetStageData(currentSelectIndex_).name.c_str());
	}
	ImGui::End();
#endif
}

void SelectScene::Draw() {
	skydome->Draw();

	fade_->Draw();
}