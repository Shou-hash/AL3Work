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

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
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