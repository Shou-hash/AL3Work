#include "SelectScene.h"
#include "AudioManager.h" // ★ BGM管理クラスのインクルード
#include "Matrix4x4.h"
#include "StageManager.h"
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

SelectScene::~SelectScene() {
	delete fade_;
	delete modelSkydome_;

	// ★ モデルの解放
	for (int32_t i = 0; i < kNumStages; ++i) {
		delete stageSprites_[i];
	}
}

void SelectScene::Initialize(StageManager* stageDataManager) {
	// ★ セレクトBGMの再生
	AudioManager::GetInstance()->PlayBGM(BGMType::kSelect);

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

	// ★ ステージ選択モデル（Stage1.obj, Stage2.obj, Stage3.obj）のロードと配置初期化
	stageSprites_[0] = KamataEngine::Model::CreateFromOBJ("Stage1", true);
	stageSprites_[1] = KamataEngine::Model::CreateFromOBJ("Stage2", true);
	stageSprites_[2] = KamataEngine::Model::CreateFromOBJ("Stage3", true);

	// 3つのステージモデルをカメラ前方（ワールド空間）に横並びで配置
	float startX = 7.0f;
	float spacing = 3.0f;
	float posY = 5.0f;
	float posZ = -7.0f;

	for (int32_t i = 0; i < kNumStages; ++i) {
		worldTransforms_[i].Initialize();
		worldTransforms_[i].translation_ = {startX + (i * spacing), posY, posZ};
		animationTimers_[i] = 0.0f;

		// ★ 回転速度を全軸（またはY軸メイン）でランダムに設定 (例: 0.02f ～ 0.07f)
		float rx = 0.02f + (static_cast<float>(rand()) / RAND_MAX) * 0.05f;
		float ry = 0.03f + (static_cast<float>(rand()) / RAND_MAX) * 0.06f;
		float rz = 0.01f + (static_cast<float>(rand()) / RAND_MAX) * 0.04f;

		// Y軸（横回転）だけにしたい場合は {0.0f, ry, 0.0f} に変更してください
		rotationSpeeds_[i] = {rx, ry, rz};
	}
	flashTimer_ = 0.0f;

	// 初期選択ステージのタイマーを満了状態にしておく
	animationTimers_[currentSelectIndex_] = 1.0f;
}

void SelectScene::Update() {
	fade_->Update();
	skydome->Update();

	camera_.UpdateMatrix();

	// 演出用の共通タイマーを進める
	flashTimer_ += 0.08f;

	// 各ステージモデルの演出パラメータ計算
	for (int32_t i = 0; i < kNumStages; ++i) {
		if (i == currentSelectIndex_) {
			// ★ 選択中：設定されたランダム速度で回転を加算
			worldTransforms_[i].rotation_.x += rotationSpeeds_[i].x;
			worldTransforms_[i].rotation_.y += rotationSpeeds_[i].y;
			worldTransforms_[i].rotation_.z += rotationSpeeds_[i].z;

			if (animationTimers_[i] < 1.0f) {
				animationTimers_[i] += 0.1f;
				if (animationTimers_[i] > 1.0f)
					animationTimers_[i] = 1.0f;
			}

			float scaleFactor = 0.8f + (animationTimers_[i] * 0.3f);
			worldTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};
		} else {
			// ★ 非選択時：回転角度を徐々に 0 (正面) に向けて衰減・復帰
			worldTransforms_[i].rotation_.x *= 0.85f;
			worldTransforms_[i].rotation_.y *= 0.85f;
			worldTransforms_[i].rotation_.z *= 0.85f;

			if (animationTimers_[i] > 0.0f) {
				animationTimers_[i] -= 0.1f;
				if (animationTimers_[i] < 0.0f)
					animationTimers_[i] = 0.0f;
			}

			float scaleFactor = 0.85f + (animationTimers_[i] * 0.25f);
			worldTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};
		}

		// ワールド行列の更新
		KamataEngine::Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransforms_[i].scale_, worldTransforms_[i].rotation_, worldTransforms_[i].translation_);
		worldTransforms_[i].matWorld_ = affineMatrix;
		worldTransforms_[i].TransferMatrix();
	}

	// シーンフェーズ管理
	switch (phase_) {
	case Phase::FadeIn:
		if (fade_->IsFinished())
			phase_ = Phase::Normal;
		break;
	case Phase::Normal:
		// 左右キーでステージ選択を変更
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_LEFT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_A)) {
			animationTimers_[currentSelectIndex_] = 0.5f;

			currentSelectIndex_--;
			if (currentSelectIndex_ < 0) {
				currentSelectIndex_ = 2;
			}

			animationTimers_[currentSelectIndex_] = 0.0f;

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_RIGHT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_D)) {
			animationTimers_[currentSelectIndex_] = 0.5f;

			currentSelectIndex_++;
			if (currentSelectIndex_ > 2) {
				currentSelectIndex_ = 0;
			}

			animationTimers_[currentSelectIndex_] = 0.0f;

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
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
			for (int32_t i = 0; i < kNumStages; ++i) {
				if (i != currentSelectIndex_)
					animationTimers_[i] = 0.0f;
			}
			animationTimers_[currentSelectIndex_] = 0.0f;

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
		ImGui::Text("Current Selection: %s", stageManager_->GetStageData(currentSelectIndex_).name.c_str());
	}
	ImGui::End();
#endif
}

void SelectScene::Draw() {
	skydome->Draw();

	// ★ ステージ選択モデル（Stage1.obj, Stage2.obj, Stage3.obj）の描画処理
	KamataEngine::Model::PreDraw();
	for (int32_t i = 0; i < kNumStages; ++i) {
		if (stageSprites_[i]) {
			stageSprites_[i]->Draw(worldTransforms_[i], camera_);
		}
	}
	KamataEngine::Model::PostDraw();

	fade_->Draw();
}