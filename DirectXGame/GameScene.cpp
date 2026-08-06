#include "GameScene.h"
#include "3d/AxisIndicator.h" // 軸方向表示ヘッダーのインクルード

using namespace KamataEngine;

GameScene::~GameScene() {
	delete model_;
	delete player_;
	delete debugCamera_;

	delete enemy_;
	delete enemyModel_;
}

void GameScene::Initialize() {
	// Inputの取得
	input_ = Input::GetInstance();

	// カメラの初期化
	camera_.Initialize();

	// デバッグカメラの生成 (画面横幅, 画面縦幅)
	debugCamera_ = new KamataEngine::DebugCamera(WinApp::kWindowWidth, WinApp::kWindowHeight);

	// 軸方向表示の設定
	// 軸方向表示の表示を有効にする
	AxisIndicator::GetInstance()->SetVisible(true);
	// 軸方向表示が参照するビュープロジェクションを指定する (アドレス渡し)
	AxisIndicator::GetInstance()->SetTargetCamera(&camera_);

	// プレイヤー初期化
	playerTex_ = TextureManager::Load("uvChecker.png");
	model_ = Model::Create();

	player_ = new Player();
	player_->Initialize(model_, playerTex_);

	// テクスチャの読み込み
	enemyTextureHandle_ = TextureManager::Load("cube.jpg");

	// モデルの作成
	enemyModel_ = Model::Create();

	// Enemy を new して初期化（第3引数に player_ を渡す）
	enemy_ = new Enemy();
	enemy_->Initialize(enemyModel_, enemyTextureHandle_, player_);
}

void GameScene::Update() {
#ifdef _DEBUG
	// 切り替えキー (P キー) でデバッグカメラ有効フラグをトグル
	if (input_->TriggerKey(DIK_P)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	// カメラの処理
	if (isDebugCameraActive_) {
		// デバッグカメラの更新
		debugCamera_->Update();

		// DebugCamera からビュー行列とプロジェクション行列を取得してコピー
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;

		// ビュープロジェクション行列の転送
		camera_.TransferMatrix();
	} else {
		// ビュープロジェクション行列の更新と転送
		camera_.UpdateMatrix();
	}

	player_->Update();

	// ポインタが null でない場合だけ更新
	if (enemy_ != nullptr) {
		enemy_->Update();
	}
}

void GameScene::Draw() 
{
	Model::PreDraw();

	player_->Draw(&camera_); 

	// ポインタが null でない場合だけ描画
	if (enemy_ != nullptr) {
		enemy_->Draw(camera_);
	}

	AxisIndicator::GetInstance()->Draw();

	Model::PostDraw();
}