#include "GameScene.h"
#include "2d/ImGuiManager.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	delete sprite_;

	delete model_;

	delete debugCamera_;
}

void GameScene::Initialize() {
	// 画像の読み込み
	textureHandle_ = TextureManager::Load("uvChecker.png");

	sprite_ = Sprite::Create(textureHandle_, {100, 50});

	modelHandle_ = TextureManager::Load("uvChecker.png");

	model_ = Model::Create();

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	camera_.Initialize();

	// サウンドデータの読み込み
	soundDataHandle_ = Audio::GetInstance()->LoadWave("fanfare.wav");

	// サウンドの再生
	Audio::GetInstance()->PlayWave(soundDataHandle_);

	voiceHandle_ = Audio::GetInstance()->PlayWave(soundDataHandle_, true);

	// ライン描画参照するカメラを指定する
	PrimitiveDrawer::GetInstance()->SetCamera(&camera_);

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	//軸方向表示の表示を有効にする
	AxisIndicator::GetInstance()->SetVisible(true);

	// 軸表示のカメラにデバッグカメラを指定する
	AxisIndicator::GetInstance()->SetTargetCamera(&debugCamera_->GetCamera());
}

void GameScene::Update() {
	// Vector2 position = sprite_->GetPosition();

	// position.x += 2.0f;
	// position.y += 1.0f;

	// sprite_->SetPosition(position);

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		// 音声停止
		Audio::GetInstance()->StopWave(voiceHandle_);
	}

	ImGui::Begin("Debug1");

#ifdef _DEBUG
	ImGui::Text("Kamata Tarou %d, %d, %d", 2050, 12, 31);
#endif

	ImGui::SliderFloat3("SliderFloat3", inputFloat, 0.0f, 1.0f);

	ImGui::ShowDemoWindow();

	ImGui::End();

	debugCamera_->Update();
}

void GameScene::Draw() {
	// Sprite::PreDraw();
	//
	// sprite_->Draw();

	// Sprite::PostDraw();

	Model::PreDraw();

	model_->Draw(worldTransform_, debugCamera_->GetCamera(), modelHandle_);

	Model::PostDraw();

	// ライン描画 (等間隔に縦線と横線を複数本描画してグリッドにする)
	for (int i = 0; i <= 10; ++i) {
		float pos = i * 2.0f; // 2.0f ごとに座標をずらす

		PrimitiveDrawer::GetInstance()->DrawLine3d({pos, 0.0f, 0.0f}, {pos, 20.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});

		PrimitiveDrawer::GetInstance()->DrawLine3d({0.0f, pos, 0.0f}, {20.0f, pos, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
	}
}