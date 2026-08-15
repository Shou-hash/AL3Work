#include "TitleScene.h"
#include "Kamataengine.h"
#include "Matrix4x4.h"
#include <cmath>
#include <numbers>

TitleScene::~TitleScene() {
	delete fade_;
	delete player_;
	delete modelPlayerHead_;
	delete modelPlayerBody_;
	delete modelPlayerLeft_;
	delete modelPlayerRight_;

	delete mapChipField_;
	delete model_;
	for (auto& worldTransformBlockLine : worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

void TitleScene::Initialize() {
	finished_ = false;
	phase_ = Phase::FadeIn;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	camera_.Initialize();
	camera_.translation_ = {10.0f, 5.0f, -15.0f};

	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipDataFromCSV("Resources/titleStageDatas.csv");
	model_ = KamataEngine::Model::CreateFromOBJ("block", true);

	uint32_t numBlockVertical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal, nullptr);
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlock) {
				KamataEngine::WorldTransform* worldTransform = new KamataEngine::WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformBlocks_[i][j] = worldTransform;
			}
		}
	}

	modelPlayerHead_ = KamataEngine::Model::CreateFromOBJ("player_head", true);
	modelPlayerBody_ = KamataEngine::Model::CreateFromOBJ("player_body", true);
	modelPlayerLeft_ = KamataEngine::Model::CreateFromOBJ("player_left", true);
	modelPlayerRight_ = KamataEngine::Model::CreateFromOBJ("player_right", true);

	player_ = new Player();
	player_->Initialize(modelPlayerBody_, &camera_, {5.0f, 4.0f, 0.0f});

	// ★ Playerにタイトルシーンのマップチップフィールドを登録
	player_->SetMapChipField(mapChipField_);
}

void TitleScene::Update() {
	fade_->Update();

	if (player_) {
		// const_castによる参照取得
		KamataEngine::WorldTransform& transformRoot = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransform());
		KamataEngine::WorldTransform& transformHead = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformHead());
		KamataEngine::WorldTransform& transformBody = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformBody());
		KamataEngine::WorldTransform& transformLeft = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformLeft());
		KamataEngine::WorldTransform& transformRight = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformRight());

		// -------------------------------------------------
		// 1. 自動右移動 & 自動ジャンプ制御（本編物理完全同期版）
		// -------------------------------------------------

		// タイトルシーン側でのローカルな物理計算(velocityYやisGroundedの二重管理)を完全に撤廃。
		// 代わりに、Playerの正規の当たり判定に必要な移動入力構造体を作成します。
		Player::CollisionMapInfo collisionMapInfo;

		// 常に右へ進む移動量（本編の移動速度 0.04f に設定）
		collisionMapInfo.moveAmount.x = 0.04f;
		collisionMapInfo.moveAmount.z = 0.0f;

		// プレイヤーの現在の「本物の接地状態」をPlayerクラスの変数などから同期・推測。
		// ※Playerクラスに `bool IsOnGround()` 等のゲッターがあれば `player_->IsOnGround()` に差し替えてください。
		// ここでは、現在の座標から足元のマップチップを直接見て本物の接地を確定させます。
		bool realGrounded = false;
		KamataEngine::Vector3 footPos = {
		    transformRoot.translation_.x,
		    transformRoot.translation_.y - 0.55f, // プレイヤーの中心から足元へのオフセット（サイズに合わせて調整）
		    transformRoot.translation_.z};
		MapChipField::IndexSet footIndex = mapChipField_->GetMapChipIndexByPosition(footPos);
		if (mapChipField_->GetMapChipTypeByIndex(footIndex.x, footIndex.y) == MapChipType::kBlock) {
			realGrounded = true;
		}

		// 本編の重力システムを適用（現在のプレイヤーのY速度に本編と同じ重力を加算、またはPlayerに任せる）
		// ここではPlayerの挙動を狂わせないため、Player内部で保持されているであろう落下速度（moveAmount.y）をベースに処理します。
		// 一旦、前回の移動結果や重力に基づいたY移動量を設定。
		static float currentVelocityY = 0.0f;
		if (!realGrounded) {
			currentVelocityY += -0.015f; // 重力
		} else {
			currentVelocityY = 0.0f; // 接地時はリセット
		}

		// ★【先読み壁検知自動ジャンプ】
		if (realGrounded) {
			// プレイヤーの少し前方（右側）の座標をシミュレート
			KamataEngine::Vector3 checkPos = {transformRoot.translation_.x + 0.6f, transformRoot.translation_.y, transformRoot.translation_.z};

			// マップチップのインデックスを取得
			MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(checkPos);

			// 目の前にブロックが存在するならジャンプ初速を与える
			MapChipType frontTile = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
			if (frontTile == MapChipType::kBlock) {
				currentVelocityY = 0.35f; // ジャンプ力
				realGrounded = false;
			}
		}

		// 確定した移動量を設定して本編の当たり判定に投げる
		collisionMapInfo.moveAmount.y = currentVelocityY;
		collisionMapInfo.onGround = realGrounded;

		// ★ プレイヤー本来のマジのマップ衝突判定を呼び出す（めり込みがここで自動補正される）
		player_->MapCollision(collisionMapInfo);

		// 衝突判定によって「めり込み補正」された正しい移動量を座標に適用
		transformRoot.translation_.x += collisionMapInfo.moveAmount.x;
		transformRoot.translation_.y += collisionMapInfo.moveAmount.y;
		transformRoot.translation_.z += collisionMapInfo.moveAmount.z;

		// プレイヤー本体の内部ステート（接地フラグなど）を同期
		player_->ApplyGroundingStatus(collisionMapInfo);

		// 実際の補正結果をローカルの速度にもフィードバック（頭をぶつけた、または着地したなど）
		currentVelocityY = collisionMapInfo.moveAmount.y;
		if (collisionMapInfo.onGround) {
			currentVelocityY = 0.0f;
		}

		// 画面右端まで行ったら左端にループ（すべての状態を安全に初期化）
		if (transformRoot.translation_.x > 40.0f) {
			transformRoot.translation_.x = 2.0f;
			transformRoot.translation_.y = 10.0f;
			currentVelocityY = 0.0f;

			collisionMapInfo.onGround = false;
			player_->ApplyGroundingStatus(collisionMapInfo);
		}

		// カメラをプレイヤーに追従させる
		camera_.translation_.x = transformRoot.translation_.x;

		// -------------------------------------------------
		// 2. アニメーション & 各パーツの行列計算
		// -------------------------------------------------
		static float titleWalkTimer = 0.0f;
		if (collisionMapInfo.onGround) {
			titleWalkTimer += 0.03f;
		} else {
			titleWalkTimer = 0.0f; // 空中ではポーズ固定
		}

		float pi = std::numbers::pi_v<float>;
		float walkTilt = std::sin(titleWalkTimer * 2.0f * pi) * 0.75f;

		if (!collisionMapInfo.onGround) {
			walkTilt = 0.3f;
		}

		transformLeft.rotation_.x = walkTilt;
		transformRight.rotation_.x = -walkTilt;

		// 行列の計算と転送
		transformRoot.matWorld_ = MakeAffineMatrix(transformRoot.scale_, transformRoot.rotation_, transformRoot.translation_);
		transformRoot.TransferMatrix();

		// 親子関係の合成
		transformBody.matWorld_ = Multiply(MakeAffineMatrix(transformBody.scale_, transformBody.rotation_, transformBody.translation_), transformRoot.matWorld_);
		transformHead.matWorld_ = Multiply(MakeAffineMatrix(transformHead.scale_, transformHead.rotation_, transformHead.translation_), transformBody.matWorld_);

		// 腕の回転中心オフセットを考慮した合成
		KamataEngine::Vector3 centerOffset = {0.0f, -0.5f, 0.0f};

		KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, transformLeft.rotation_, {0.0f, 0.0f, 0.0f});
		KamataEngine::Matrix4x4 preTranslateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
		KamataEngine::Vector3 finalTranslationLeft = {transformLeft.translation_.x + centerOffset.x, transformLeft.translation_.y + centerOffset.y, transformLeft.translation_.z + centerOffset.z};
		KamataEngine::Matrix4x4 postTranslateLeft = MakeAffineMatrix(transformLeft.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationLeft);
		transformLeft.matWorld_ = Multiply(postTranslateLeft, Multiply(rotateLeft, preTranslateLeft));
		transformLeft.matWorld_ = Multiply(transformLeft.matWorld_, transformBody.matWorld_);

		KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, transformRight.rotation_, {0.0f, 0.0f, 0.0f});
		KamataEngine::Matrix4x4 preTranslateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
		KamataEngine::Vector3 finalTranslationRight = {transformRight.translation_.x + centerOffset.x, transformRight.translation_.y + centerOffset.y, transformRight.translation_.z + centerOffset.z};
		KamataEngine::Matrix4x4 postTranslateRight = MakeAffineMatrix(transformRight.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationRight);
		transformRight.matWorld_ = Multiply(postTranslateRight, Multiply(rotateRight, preTranslateRight));
		transformRight.matWorld_ = Multiply(transformRight.matWorld_, transformBody.matWorld_);

		// 行列転送
		transformBody.TransferMatrix();
		transformHead.TransferMatrix();
		transformLeft.TransferMatrix();
		transformRight.TransferMatrix();
	}

	// タイトルステージのブロック行列の更新処理
	for (auto& worldTransformBlockLine : worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->TransferMatrix();
		}
	}

	camera_.UpdateMatrix();

	// シーンフェーズ管理
	switch (phase_) {
	case Phase::FadeIn:
		if (fade_->IsFinished())
			phase_ = Phase::Normal;
		break;
	case Phase::Normal:
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::FadeOut;
		}
		break;
	case Phase::FadeOut:
		if (fade_->IsFinished())
			finished_ = true;
		break;
	}
}

void TitleScene::Draw() {
	for (auto& worldTransformBlockLine : worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			KamataEngine::Model::PreDraw();
			model_->Draw(*worldTransformBlock, camera_);
			KamataEngine::Model::PostDraw();
		}
	}

	if (player_) {
		KamataEngine::Model::PreDraw();
		player_->Draw();
		KamataEngine::Model::PostDraw();
	}
	fade_->Draw();
}