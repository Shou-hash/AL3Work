#include "SelectScene.h"
#include "AudioManager.h" // ★ BGM管理クラスのインクルード
#include "Matrix4x4.h"
#include "StageManager.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

SelectScene::~SelectScene() {
	delete fade_;
	delete modelSkydome_;
	delete modelArrow_; // ★ 矢印モデルの解放

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

	// ★ 操作解説用矢印モデル（Arrow.obj）のロード
	modelArrow_ = KamataEngine::Model::CreateFromOBJ("Arrow", true);

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

		rotationSpeeds_[i] = {rx, ry, rz};
	}
	flashTimer_ = 0.0f;

	// ★ 左右の操作解説矢印の配置初期化（0: 左端矢印, 1: 右端矢印）
	for (int32_t i = 0; i < 2; ++i) {
		arrowTransforms_[i].Initialize();
		arrowAnimationTimers_[i] = 0.0f;
		arrowTransforms_[i].scale_ = {1.0f, 1.0f, 1.0f};
	}

	float arrowPosY = posY - 2.0f;
	arrowTransforms_[0].translation_ = {6.5f, arrowPosY, posZ};  // 左矢印
	arrowTransforms_[1].translation_ = {13.5f, arrowPosY, posZ}; // 右矢印

	// 矢印モデルの向き（0: 左向き Z=0, 1: 右向き Z=180度）
	arrowTransforms_[0].rotation_ = {0.0f, 0.0f, 0.0f};
	arrowTransforms_[1].rotation_ = {0.0f, 0.0f, std::numbers::pi_v<float>};

	for (int32_t i = 0; i < 2; ++i) {
		KamataEngine::Matrix4x4 arrowMatrix = MakeAffineMatrix(arrowTransforms_[i].scale_, arrowTransforms_[i].rotation_, arrowTransforms_[i].translation_);
		arrowTransforms_[i].matWorld_ = arrowMatrix;
		arrowTransforms_[i].TransferMatrix();
	}

	// 初期選択ステージのタイマーを満了状態にしておく
	animationTimers_[currentSelectIndex_] = 1.0f;

	// ★ 朝のライティングを設定（ラスボスの夜・赤いスポットライトは使わない）
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize();
	lightManager_->ApplyToModel(modelSkydome_, LightManager::GroupType::kWorld);
	for (int32_t i = 0; i < kNumStages; ++i) {
		lightManager_->ApplyToModel(stageSprites_[i], LightManager::GroupType::kWorld);
	}
	if (modelArrow_) {
		lightManager_->ApplyToModel(modelArrow_, LightManager::GroupType::kWorld);
	}
}

void SelectScene::Update() {
	fade_->Update();
	skydome->Update();

	camera_.UpdateMatrix();

	// ★ 朝のライティングの更新
	lightManager_->Update();

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

	// ★ 左右矢印のキー押下による Scale アニメーションの更新
	for (int32_t i = 0; i < 2; ++i) {
		if (arrowAnimationTimers_[i] > 0.0f) {
			arrowAnimationTimers_[i] -= 0.12f;
			if (arrowAnimationTimers_[i] < 0.0f) {
				arrowAnimationTimers_[i] = 0.0f;
			}
		}

		// 押した瞬間にポンッと拡大して滑らかに元の大きさに戻るアニメーション
		float scaleFactor = 1.0f + std::sin(arrowAnimationTimers_[i] * std::numbers::pi_v<float>) * 0.45f;
		arrowTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};

		KamataEngine::Matrix4x4 arrowMatrix = MakeAffineMatrix(arrowTransforms_[i].scale_, arrowTransforms_[i].rotation_, arrowTransforms_[i].translation_);
		arrowTransforms_[i].matWorld_ = arrowMatrix;
		arrowTransforms_[i].TransferMatrix();
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
			arrowAnimationTimers_[0] = 1.0f; // ★ 左矢印のアニメーション開始

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
			arrowAnimationTimers_[1] = 1.0f; // ★ 右矢印のアニメーション開始

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}

		// ★ 決定キー（SPACE）でステージ確定 & カメラ演出開始
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			if (stageManager_) {
				stageManager_->SetCurrentStageIndex(currentSelectIndex_);
			}

			// カメラ演出の初期設定
			phase_ = Phase::SelectPerformance;
			cameraPerformanceTimer_ = 0.0f;
			cameraStartPos_ = camera_.translation_;

			// 選択されたステージモデルの位置に寄る目標座標を計算
			const KamataEngine::Vector3& stagePos = worldTransforms_[currentSelectIndex_].translation_;
			cameraTargetPos_ = {stagePos.x, stagePos.y, stagePos.z - 3.8f};

			// ★ ステージ確定SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
		break;

	case Phase::SelectPerformance: {
		// ★ 確定時のステージモデルへのカメラ接近演出
		cameraPerformanceTimer_ += 1.0f / 60.0f;
		constexpr float kPerformanceDuration = 0.8f;
		float t = std::clamp(cameraPerformanceTimer_ / kPerformanceDuration, 0.0f, 1.0f);

		// イージング (EaseOutCubic) で滑らかに接近
		float easeVal = 1.0f - std::pow(1.0f - t, 3.0f);

		camera_.translation_.x = cameraStartPos_.x + (cameraTargetPos_.x - cameraStartPos_.x) * easeVal;
		camera_.translation_.y = cameraStartPos_.y + (cameraTargetPos_.y - cameraStartPos_.y) * easeVal;
		camera_.translation_.z = cameraStartPos_.z + (cameraTargetPos_.z - cameraStartPos_.z) * easeVal;

		// 選択されたステージモデルの回転速度を加速させてアピール演出
		worldTransforms_[currentSelectIndex_].rotation_.y += 0.12f;

		// カメラ演出が完了したらフェードアウト開始
		if (t >= 1.0f) {
			fade_->Start(Fade::Status::FadeOut, 0.8f);
			phase_ = Phase::FadeOut;
		}
		break;
	}

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

	// ★ ライティング（光沢の強さなど）の調整
	lightManager_->DrawImGui();
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

	// ★ 左右端の操作解説矢印モデルの描画処理
	if (modelArrow_) {
		for (int32_t i = 0; i < 2; ++i) {
			modelArrow_->Draw(arrowTransforms_[i], camera_);
		}
	}
	KamataEngine::Model::PostDraw();

	fade_->Draw();
}