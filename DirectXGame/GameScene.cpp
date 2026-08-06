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

void GameScene::CheckAllCollisions() {
	// 判定対象AとBの座標
	Vector3 posA, posB;

	// 自弾リストの取得
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();
	// 敵弾リストの取得
	const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();

#pragma region 自キャラと敵弾の当たり判定
	{
		// 自キャラの座標
		posA = player_->GetWorldPosition();

		// 半径の設定（見た目に応じて適宜変更してください）
		const float kPlayerRadius = 1.0f;
		const float kEnemyBulletRadius = 0.5f;

		// 自キャラと敵弾全ての当たり判定
		for (EnemyBullet* bullet : enemyBullets) {
			if (bullet->IsDead()) continue;

			// 敵弾の座標
			posB = bullet->GetWorldPosition();

			// 2点間の距離の2乗を計算
			float dx = posB.x - posA.x;
			float dy = posB.y - posA.y;
			float dz = posB.z - posA.z;
			float distSquare = dx * dx + dy * dy + dz * dz;

			// 半径の和の2乗
			float radiusSum = kPlayerRadius + kEnemyBulletRadius;
			float radiusSumSquare = radiusSum * radiusSum;

			// 球同士の判定 ( (x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2 <= (R1+R2)^2 )
			if (distSquare <= radiusSumSquare) {
				// 自キャラの衝突時コールバックを呼び出す
				player_->OnCollision();
				// 敵弾の衝突時コールバックを呼び出す
				bullet->OnCollision();
			}
		}
	}
#pragma endregion

#pragma region 自弾と敵キャラの当たり判定
	{
		// 敵キャラの座標
		posB = enemy_->GetWorldPosition();

		const float kPlayerBulletRadius = 0.5f;
		const float kEnemyRadius = 1.0f;

		// 自弾全てと敵キャラの当たり判定
		for (PlayerBullet* bullet : playerBullets) {
			if (bullet->IsDead()) continue;

			// 自弾の座標
			posA = bullet->GetWorldPosition();

			// 2点間の距離の2乗を計算
			float dx = posB.x - posA.x;
			float dy = posB.y - posA.y;
			float dz = posB.z - posA.z;
			float distSquare = dx * dx + dy * dy + dz * dz;

			float radiusSum = kPlayerBulletRadius + kEnemyRadius;
			float radiusSumSquare = radiusSum * radiusSum;

			// 球同士の判定
			if (distSquare <= radiusSumSquare) {
				// 自弾の衝突時コールバックを呼び出す
				bullet->OnCollision();
				// 敵キャラの衝突時コールバックを呼び出す
				enemy_->OnCollision();
			}
		}
	}
#pragma endregion

#pragma region 自弾と敵弾の当たり判定
	{
		const float kPlayerBulletRadius = 0.5f;
		const float kEnemyBulletRadius = 0.5f;

		// 総当たり（二重for文）で判定
		for (PlayerBullet* pBullet : playerBullets) {
			if (pBullet->IsDead()) continue;

			for (EnemyBullet* eBullet : enemyBullets) {
				if (eBullet->IsDead()) continue;

				posA = pBullet->GetWorldPosition();
				posB = eBullet->GetWorldPosition();

				// 2点間の距離の2乗を計算
				float dx = posB.x - posA.x;
				float dy = posB.y - posA.y;
				float dz = posB.z - posA.z;
				float distSquare = dx * dx + dy * dy + dz * dz;

				float radiusSum = kPlayerBulletRadius + kEnemyBulletRadius;
				float radiusSumSquare = radiusSum * radiusSum;

				// 球同士の判定
				if (distSquare <= radiusSumSquare) {
					// 自弾と敵弾の衝突時コールバックを呼び出す
					pBullet->OnCollision();
					eBullet->OnCollision();
				}
			}
		}
	}
#pragma endregion
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