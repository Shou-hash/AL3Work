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
	AxisIndicator::GetInstance()->SetVisible(true);
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

	// ★ 当たり判定のチェック
	CheckAllCollisions();
}

void GameScene::Draw() {
	Model::PreDraw();

	player_->Draw(&camera_);

	// ポインタが null でない場合だけ描画
	if (enemy_ != nullptr) {
		enemy_->Draw(camera_);
	}

	AxisIndicator::GetInstance()->Draw();

	Model::PostDraw();
}

/// <summary>
/// 2つのコライダー間の距離判定とコールバック呼び出し
/// </summary>
void GameScene::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {
	// コライダーA, Bのワールド座標を取得
	Vector3 posA = colliderA->GetWorldPosition();
	Vector3 posB = colliderB->GetWorldPosition();

	// 2点間の距離の2乗を計算
	float dx = posB.x - posA.x;
	float dy = posB.y - posA.y;
	float dz = posB.z - posA.z;
	float distSquare = dx * dx + dy * dy + dz * dz;

	// 半径の和の2乗
	float radiusSum = colliderA->GetRadius() + colliderB->GetRadius();
	float radiusSumSquare = radiusSum * radiusSum;

	// 球同士の判定 ( (x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2 <= (R1+R2)^2 )
	if (distSquare <= radiusSumSquare) {
		// それぞれの OnCollision コールバックを呼び出す
		colliderA->OnCollision();
		colliderB->OnCollision();
	}
}

/// <summary>
/// 全ての当たり判定
/// </summary>
void GameScene::CheckAllCollisions() {
	// 自弾リストの取得
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();
	// 敵弾リストの取得
	const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();

#pragma region 自キャラと敵弾の当たり判定
	for (EnemyBullet* bullet : enemyBullets) {
		if (bullet->IsDead())
			continue;

		CheckCollisionPair(player_, bullet);
	}
#pragma endregion

#pragma region 自弾と敵キャラの当たり判定
	for (PlayerBullet* bullet : playerBullets) {
		if (bullet->IsDead())
			continue;

		CheckCollisionPair(bullet, enemy_);
	}
#pragma endregion

#pragma region 自弾と敵弾の当たり判定
	for (PlayerBullet* pBullet : playerBullets) {
		if (pBullet->IsDead())
			continue;

		for (EnemyBullet* eBullet : enemyBullets) {
			if (eBullet->IsDead())
				continue;

			CheckCollisionPair(pBullet, eBullet);
		}
	}
#pragma endregion
}