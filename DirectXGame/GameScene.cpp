#include "GameScene.h"
#include "3d/AxisIndicator.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	delete model_;
	delete player_;
	delete debugCamera_;

	delete enemy_;
	delete enemyModel_;

	// ★ 衝突マネージャの解放を忘れずに実行
	delete collisionManager_;

	// 天球の解放
	delete skydome_;
	delete modelSkydome_;
}

void GameScene::Initialize() {
	input_ = Input::GetInstance();
	camera_.Initialize();

	// 最初の通常の視角を上から見下ろす角度（X軸90度回転 = 1.57079f）に設定
	camera_.rotation_ = {1.57079f, 0.0f, 0.0f};
	// 上から見下ろしたときに見えやすいよう初期位置の高さ(Y)と奥行き(Z)を少し調整
	camera_.translation_ = {0.0f, 50.0f, -10.0f};

	debugCamera_ = new KamataEngine::DebugCamera(WinApp::kWindowWidth, WinApp::kWindowHeight);

	AxisIndicator::GetInstance()->SetVisible(true);
	AxisIndicator::GetInstance()->SetTargetCamera(&camera_);

	playerTex_ = TextureManager::Load("uvChecker.png");
	model_ = Model::Create();

	player_ = new Player();
	player_->Initialize(model_, playerTex_);

	enemyTextureHandle_ = TextureManager::Load("cube.jpg");
	enemyModel_ = Model::Create();

	enemy_ = new Enemy();
	enemy_->Initialize(enemyModel_, enemyTextureHandle_, player_);

	// ★ 衝突マネージャの生成
	collisionManager_ = new CollisionManager();

	// 3Dモデルの生成
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	// 天球の生成と初期化
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_, &camera_);
}

void GameScene::Update() {
#ifdef _DEBUG
	if (input_->TriggerKey(DIK_P)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	// マウスホイールによる遠近変更処理（ホイールの回転量を反映）
	int32_t wheel = input_->GetWheel();
	if (wheel != 0) {
		// ホイールの回転方向に応じて距離を増減（感度は 0.01f で調整）
		camera_.translation_.z += wheel * 0.01f;
		camera_.translation_.y -= wheel * 0.01f; // 見下ろし視点時に近づくよう調整
	}

	if (isDebugCameraActive_) {
		// マウス左長押し中にデバッグカメラの更新処理（移動・回転等）を行う
		if (input_->IsPressMouse(0)) {
			debugCamera_->Update();
		}

		// Cキーを押すと視角が元の角度に直すようにしてほしい、そして元の角度を上から見下ろすようにしてほしい
		if (input_->TriggerKey(DIK_C)) {
			// 読み取り専用制約を回避するため、一度デバッグカメラのメモリを破棄して再生成
			delete debugCamera_;
			debugCamera_ = new KamataEngine::DebugCamera(WinApp::kWindowWidth, WinApp::kWindowHeight);

			// 再初期化後に一度強制更新をかけてビュー行列を作成する
			debugCamera_->Update();

			// Cキーを押した後のカメラの回転角を真上からの見下ろしに強制リセット
			camera_.rotation_ = {1.57079f, 0.0f, 0.0f};
			camera_.translation_ = {0.0f, 50.0f, -10.0f};
			camera_.UpdateMatrix();
		} else {
			// Cキーが押されていない通常時はデバッグカメラの行列をそのまま適用
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;
			camera_.TransferMatrix();
		}
	} else {
		camera_.UpdateMatrix();
	}

	// 天球の更新
	skydome_->Update();

	player_->Update();

	if (enemy_ != nullptr) {
		enemy_->Update();
	}

	// ----------------------------------------------------
	// ★ 衝突判定マネージョへの委託処理 (毎フレーム実行)
	// ----------------------------------------------------

	// 1. 前フレームの情報を一度クリアする
	collisionManager_->ClearColliders();

	// 2. 自キャラの登録
	collisionManager_->AddCollider(player_);

	// 3. 自弾リストの登録
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();
	for (PlayerBullet* bullet : playerBullets) {
		if (!bullet->IsDead()) {
			collisionManager_->AddCollider(bullet);
		}
	}

	// 4. 敵キャラの登録 (ポインタが存在する場合)
	if (enemy_ != nullptr) {
		collisionManager_->AddCollider(enemy_);

		// 5. 敵弾リストの登録
		const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();
		for (EnemyBullet* bullet : enemyBullets) {
			if (!bullet->IsDead()) {
				collisionManager_->AddCollider(bullet);
			}
		}
	}

	// 6. マネージャに集約したコライダー全体の当たり判定を一括実行
	collisionManager_->CheckAllCollisions();

	// ----------------------------------------------------
}

void GameScene::Draw() {
	Model::PreDraw();

	// 天球の描画
	skydome_->Draw();

	player_->Draw(&camera_);

	if (enemy_ != nullptr) {
		enemy_->Draw(camera_);
	}

	AxisIndicator::GetInstance()->Draw();

	Model::PostDraw();
}