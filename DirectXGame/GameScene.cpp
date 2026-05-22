#define _USE_MATH_DEFINES
#include "GameScene.h"
#include "Easeing.h"
#include <cmath>
#include <ctime>
#include <math.h>
#include <string>
#include <time.h>

using namespace KamataEngine;

// ギヤの初期配置データ
struct GearSpawnData {
	Vector2 position;
	float radius;
	float rotateSpeed;
	float grSize;
	int textureIndex;
};

float GameScene::Random(float min, float max) { return min + (float)std::rand() / RAND_MAX * (max - min); }

const GearSpawnData kGearInitDatas[kGearNum] = {
    {{168.0f, -24.0f},  160.0f, 0.03f,  320, 4 },
    {{164.0f, 136.0f},  144.0f, -0.10f, 288, 1 },
    {{298.0f, 282.0f},  90.0f,  0.06f,  180, 3 },
    {{198.0f, 346.0f},  90.0f,  -0.09f, 180, 7 },
    {{478.0f, -10.0f},  90.0f,  0.02f,  180, 7 },
    {{42.0f, 470.0f},   90.0f,  -0.01f, 180, 2 },
    {{154.0f, 500.0f},  96.0f,  -0.1f,  192, 0 },
    {{156.0f, 944.0f},  220.0f, 0.03f,  440, 8 },
    {{196.0f, 852.0f},  96.0f,  -0.07f, 192, 0 },
    {{230.0f, 986.0f},  90.0f,  -0.04f, 180, 7 },
    {{116.0f, 964.0f},  144.0f, 0.02f,  288, 1 },
    {{492.0f, 1088.0f}, 220.0f, -0.07f, 440, 5 },
    {{692.0f, 1208.0f}, 220.0f, -0.05f, 440, 8 },
    {{1468.0f, 12.0f},  160.0f, 0.10f,  320, 4 },
    {{1402.0f, 66.0f},  90.0f,  -0.02f, 180, 7 },
    {{1616.0f, 312.0f}, 220.0f, -0.11f, 440, 5 },
    {{1500.0f, 100.0f}, 96.0f,  0.06f,  192, 0 },
    {{1804.0f, 584.0f}, 110.0f, -0.08f, 220, 6 },
    {{1660.0f, 992.0f}, 220.0f, -0.03f, 440, 9 },
    {{1324.0f, 980.0f}, 96.0f,  0.03f,  192, 0 },
    {{1666.0f, 554.0f}, 90.0f,  -0.06f, 180, 7 },
    {{1526.0f, 994.0f}, 110.0f, -0.12f, 220, 10},
    {{1406.0f, 950.0f}, 90.0f,  0.04f,  180, 7 },
    {{1744.0f, 932.0f}, 144.0f, -0.01f, 288, 1 }
};

GameScene::~GameScene() {
	for (int i = 0; i < 3; i++) {
		delete sprBg_[i];
	}
	for (int i = 0; i < 3; i++) {
		delete sprClock_[i];
	}
	delete sprHandHour_;
	delete sprHandMin_;

	for (int i = 0; i < kPieceNum; i++) {
		delete pieces_[i].sprite;
	}
	for (int i = 0; i < kSparkleNum; i++) {
		delete sparkles_[i].sprite;
	}

	for (int i = 0; i < 11; i++) {
		delete sprGear_[i];
	}
	delete sprSun_;
	delete sprMoon_;
	for (int i = 0; i < 2; i++) {
		delete sprSpace_[i];
	}
}

void GameScene::Initialize() {
	std::srand((unsigned int)std::time(nullptr));
	camera_.Initialize();

	for (int i = 0; i < 3; i++) {
		texBg_[i] = TextureManager::Load("./Resource/Clock/Bg/bg" + std::to_string(i + 1) + ".png");
		if (texBg_[i])
			sprBg_[i] = Sprite::Create(texBg_[i], {0, 0});
	}

	for (int i = 0; i < 3; i++) {
		texClock_[i] = TextureManager::Load("./Resource/Clock/clock" + std::to_string(i + 1) + ".png");
		if (texClock_[i]) {
			sprClock_[i] = Sprite::Create(texClock_[i], {0, 0});
			sprClock_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	texHandHour_ = TextureManager::Load("./Resource/Clock/Hand/hourHand.png");
	if (texHandHour_) {
		sprHandHour_ = Sprite::Create(texHandHour_, {0, 0});
		sprHandHour_->SetAnchorPoint({0.0f, 0.5f}); // ★時針の回転中心
	}
	texHandMin_ = TextureManager::Load("./Resource/Clock/Hand/minHand.png");
	if (texHandMin_) {
		sprHandMin_ = Sprite::Create(texHandMin_, {0, 0});
		sprHandMin_->SetAnchorPoint({0.0f, 0.5f}); // ★分針の回転中心
	}

	for (int i = 0; i < 4; i++) {
		texPiece_[i] = TextureManager::Load("./Resource/Clock/Piece/piece" + std::to_string(i + 1) + ".png");
	}
	texSparkle_ = TextureManager::Load("./Resource/Clock/Light/particle.png");

	for (int i = 0; i < 11; i++) {
		texGear_[i] = TextureManager::Load("./Resource/Gear/gear" + std::to_string(i + 1) + ".png");
		if (texGear_[i]) {
			sprGear_[i] = Sprite::Create(texGear_[i], {0, 0});
			sprGear_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	texSun_ = TextureManager::Load("./Resource/Clock/Light/sun2.png");
	if (texSun_) {
		sprSun_ = Sprite::Create(texSun_, {0, 0});
		sprSun_->SetAnchorPoint({0.5f, 0.5f});
	}

	texMoon_ = TextureManager::Load("./Resource/Clock/Light/month2.png");
	if (texMoon_) {
		sprMoon_ = Sprite::Create(texMoon_, {0, 0});
		sprMoon_->SetAnchorPoint({0.5f, 0.5f});
	}

	// Space UI の読み込み
	for (int i = 0; i < 2; i++) {
		texSpace_[i] = TextureManager::Load("./Resource/Clock/space" + (i == 0 ? std::string("") : std::to_string(2)) + ".png");
		if (texSpace_[i]) {
			sprSpace_[i] = Sprite::Create(texSpace_[i], {0, 0});
			sprSpace_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	for (int i = 0; i < kGearNum; i++) {
		gears_[i].position = kGearInitDatas[i].position;
		gears_[i].radius = kGearInitDatas[i].radius;
		gears_[i].rotateSpeed = kGearInitDatas[i].rotateSpeed;
		gears_[i].size = kGearInitDatas[i].grSize;
		gears_[i].textureIndex = kGearInitDatas[i].textureIndex;
		gears_[i].angle = 0.0f;
		// ギヤのアニメーション変数を初期化
		gears_[i].startupTimer = 0.0f;
		gears_[i].isStadyRotation = false;
		gears_[i].shakeAmount = 0.02f;
		gears_[i].shakeSpeed = 30.0f;
	}

	for (int i = 0; i < kPieceNum; i++) {
		pieces_[i].position = {960.0f, 540.0f};
		pieces_[i].velocity = {Random(-3.0f, 3.0f), Random(-2.0f, -6.0f)};
		pieces_[i].angle = Random(0.0f, 6.28f);
		pieces_[i].angularVelocity = Random(-0.05f, 0.05f);
		pieces_[i].isDisplay = false;
		pieces_[i].width = Random(10.0f, 48.0f);
		pieces_[i].height = Random(10.0f, 48.0f);
		pieces_[i].life = 0.0f;
		pieces_[i].maxLife = 1.0f;
		pieces_[i].textureIndex = std::rand() % kPieceTexNum;

		// 個別にスプライトを作成する
		if (texPiece_[pieces_[i].textureIndex]) {
			pieces_[i].sprite = Sprite::Create(texPiece_[pieces_[i].textureIndex], {0, 0});
			pieces_[i].sprite->SetAnchorPoint({0.5f, 0.5f});
		} else {
			pieces_[i].sprite = nullptr;
		}
	}

	for (int i = 0; i < kSparkleNum; i++) {
		sparkles_[i].isDisplay = false;
		if (texSparkle_) {
			sparkles_[i].sprite = Sprite::Create(texSparkle_, {0, 0});
			sparkles_[i].sprite->SetAnchorPoint({0.5f, 0.5f});
		} else {
			sparkles_[i].sprite = nullptr;
		}
	}
}

void GameScene::Update() {
	Input* input = Input::GetInstance();

	hourCos_ = cosf(hourAngle_);
	spaceAnimTimer_++;

	#pragma region 拡大縮小更新処理

	// スペースキーの状態
	if (input->PushKey(DIK_SPACE)) {
		isExpanding_ = true;
	} else {
		isExpanding_ = false;
	}

	// --- 盤面2の更新 (先に動く) ---
	if (isExpanding_) {
		scaleTimer2_ += kScaleSpeed_;
	} else {
		scaleTimer2_ -= kScaleSpeed_;
	}

	if (scaleTimer2_ > 1.0f) {
		scaleTimer2_ = 1.0f;
	}
	if (scaleTimer2_ < 0.0f) {
		scaleTimer2_ = 0.0f;
	}

	// --- 盤面1の更新 (盤面2が0.3以上進んだら動き出すディレイ処理) ---
	// 拡大時：盤面2がある程度進んだら開始
	// 縮小時：盤面2がある程度戻ったら開始
	if (isExpanding_) {
		if (scaleTimer2_ > 0.3f) { // 0.3秒分のディレイ
			scaleTimer1_ += kScaleSpeed_;
		}
	} else {
		if (scaleTimer2_ < 0.7f) { // 戻る時も差をつける
			scaleTimer1_ -= kScaleSpeed_;
		}
	}

	if (scaleTimer1_ > 1.0f) {
		scaleTimer1_ = 1.0f;
	}
	if (scaleTimer1_ < 0.0f) {
		scaleTimer1_ = 0.0f;
	}

	// 個別にイージングを適用
	float scaleT1 = EaseInOutQuart(scaleTimer1_);
	float scaleT2 = EaseInOutQuart(scaleTimer2_);

	// 倍率の計算
	currentScale1_ = 1.0f + (kMaxScale_ - 1.35f) * scaleT1;
	currentScale2_ = 1.0f + (kMaxScale_ - 1.35f) * scaleT2;

#pragma endregion

	#pragma region 時計回転ロジック

	if (!isRotating_) {
		bool isSpacePressed = input->TriggerKey(DIK_SPACE);
		bool isAPressed = input->TriggerKey(DIK_A);
		bool isDPressed = input->TriggerKey(DIK_D);
		intervalTimer_++;

		if (intervalTimer_ > 120 || isSpacePressed || isAPressed || isDPressed) {
			isRotating_ = true;
			easeTimer_ = 0.0f;
			minStart_ = minAngle_;
			hourStart_ = hourAngle_;

			const float kToRad = float(M_PI) / 180.0f;
			float moveAngleMin = 30.0f * kToRad;
			float moveAngleHour = 2.5f * kToRad;

			if (isSpacePressed) {
				minTarget_ = minAngle_ - moveAngleMin;
				hourTarget_ = hourAngle_ - moveAngleHour;
			}
			else if (isAPressed) {
				hourTarget_ = hourAngle_ - moveAngleMin;
			}
			else if (isDPressed) {
				hourTarget_ = hourAngle_ + moveAngleMin;
			}
			else {
				minTarget_ = minAngle_ + moveAngleMin;
				hourTarget_ = hourAngle_ + moveAngleHour;
			}

			shakeTimer_ = 30.0f;
			intervalTimer_ = 0;

			// 欠片の放出とライフの初期化
			for (int i = 0; i < kPieceNum; i++) {
				if (!pieces_[i].isDisplay) {
					pieces_[i].isDisplay = true;
					pieces_[i].life = Random(60.0f, 150.0f); // 寿命を設定
					pieces_[i].maxLife = pieces_[i].life;

					// 三等分して出現位置と飛ぶ方向を分ける
					if (i < kPieceNum / 3) {
						// ① 左端側（全体の1/3）: Xは0、Yは0〜720
						pieces_[i].position = {0.0f, Random(0.0f, 720.0f)};
						// 右（画面内）へ向かって飛ぶように速度調整
						pieces_[i].velocity = {Random(2.0f, 6.0f), Random(-6.0f, -1.0f)};
					} else if (i < (kPieceNum / 3) * 2) {
						// ② 右端側（全体の1/3）: Xは1280、Yは0〜720
						pieces_[i].position = {1280.0f, Random(0.0f, 720.0f)};
						// 左（画面内）へ向かって飛ぶように速度調整
						pieces_[i].velocity = {Random(-6.0f, -2.0f), Random(-6.0f, -1.0f)};
					} else {
						// ③ 上側（全体の1/3）: Xは0〜1280、Yは0
						pieces_[i].position = {Random(0.0f, 1280.0f), 0.0f};
						// 下へ向かって落ちるように速度調整（重力と合わせて加速します）
						pieces_[i].velocity = {Random(-3.0f, 3.0f), Random(0.0f, 3.0f)};
					}
					pieces_[i].angularVelocity = Random(-0.1f, 0.1f);
				}
			}
		}
	} else {
		easeTimer_ += 1.0f / 30.0f;
		if (easeTimer_ >= 1.0f) {
			isRotating_ = false;
			minAngle_ = minTarget_;
			hourAngle_ = hourTarget_;
		} else {
			float easeVal = EaseOutQuart(easeTimer_);
			minAngle_ = minStart_ + (minTarget_ - minStart_) * easeVal;
			hourAngle_ = hourStart_ + (hourTarget_ - hourStart_) * easeVal;
		}
	}

	#pragma endregion

	// --- 時計パルス演出（拡大縮小） ---
	if (input->TriggerKey(DIK_SPACE)) {
		const float kScaleSpeed = 1.0f / 20.0f;
		if (isExpanding_) {
			scaleTimer_ += kScaleSpeed;
			if (scaleTimer_ >= 1.0f)
				isExpanding_ = false;
		} else {
			scaleTimer_ -= kScaleSpeed;
			if (scaleTimer_ <= 0.0f)
				isExpanding_ = true;
		}
	} else {
		scaleTimer_ = 0.0f;
		isExpanding_ = false;
	}
	currentScale_ = 1.0f + (1.2f - 1.0f) * EaseInOutQuart(scaleTimer_);

	// 光(星)の放出
	for (int i = 0; i < kSparkleNum; i++) {
		if (!sparkles_[i].isDisplay) {
			sparkles_[i].isDisplay = true;
			sparkles_[i].position.x = Random(0.0f, 1920.0f);
			sparkles_[i].position.y = Random(0.0f, 1080.0f);
			float angle = Random(0.0f, float(M_PI) * 2.0f);
			float speed = Random(1.0f, 4.0f);
			sparkles_[i].velocity.x = cosf(angle) * speed;
			sparkles_[i].velocity.y = sinf(angle) * speed;
			sparkles_[i].alpha = 1.0f;
			sparkles_[i].lifeSpeed = Random(0.01f, 0.03f);
		}
	}

	for (int i = 0; i < kSparkleNum; i++) {
		if (sparkles_[i].isDisplay) {
			sparkles_[i].position.x += sparkles_[i].velocity.x;
			sparkles_[i].position.y += sparkles_[i].velocity.y;
			sparkles_[i].alpha -= sparkles_[i].lifeSpeed;
			if (sparkles_[i].alpha <= 0.0f)
				sparkles_[i].isDisplay = false;
		}
	}

	if (shakeTimer_ > 0) {
		shakeTimer_ -= 1.0f;
		shakeOffset_ = {Random(-5, 5), Random(-5, 5)};
	} else {
		shakeOffset_ = {0, 0};
	}

	// ギヤのアニメーションと回転更新
	for (int i = 0; i < kGearNum; i++) {
		if (!gears_[i].isStadyRotation) {
			gears_[i].startupTimer += 0.02f;
			if (gears_[i].startupTimer >= 1.0f) {
				gears_[i].startupTimer = 1.0f;
				gears_[i].isStadyRotation = true;
			}
			// イージングをかけて回転速度を徐々に上げる
			float easeT = EaseOutBounce(gears_[i].startupTimer);
			gears_[i].angle += gears_[i].rotateSpeed * easeT;

			// 起動時の微小な揺れ
			gears_[i].position.x = kGearInitDatas[i].position.x + sinf(gears_[i].startupTimer * gears_[i].shakeSpeed) * gears_[i].shakeAmount * 100.0f;
		} else {
			gears_[i].angle += gears_[i].rotateSpeed;
			gears_[i].position.x = kGearInitDatas[i].position.x; // 位置リセット
		}
	}

	// 欠片の物理更新と寿命管理
	for (int i = 0; i < kPieceNum; i++) {
		if (pieces_[i].isDisplay) {
			pieces_[i].position.x += pieces_[i].velocity.x;
			pieces_[i].position.y += pieces_[i].velocity.y;
			pieces_[i].velocity.y += 0.4f; // 重力
			pieces_[i].angle += pieces_[i].angularVelocity;
			pieces_[i].life -= 1.0f;

			if (pieces_[i].position.y > 1080.0f || pieces_[i].life <= 0.0f) {
				pieces_[i].isDisplay = false;
			}
		}
	}
}

void GameScene::Draw() {
	Sprite::PreDraw();

	Vector2 drawPos = {clockPos_.x + shakeOffset_.x, clockPos_.y + shakeOffset_.y};

	if (sprBg_[0])
		sprBg_[0]->Draw();

	// 昼夜の表現をhourCos_を元に描画
	if (hourCos_ >= 0) {
		sprSun_->SetPosition({640.0f + shakeOffset_.x, 360.0f + shakeOffset_.y});
		sprSun_->SetSize({sprSun_->GetTextureSize().x * currentScale_, sprSun_->GetTextureSize().y * currentScale_});
		sprSun_->Draw();
	} else {
		sprMoon_->SetPosition({640.0f + shakeOffset_.x, 360.0f + shakeOffset_.y});
		sprMoon_->SetSize({sprMoon_->GetTextureSize().x * currentScale_, sprMoon_->GetTextureSize().y * currentScale_});
		sprMoon_->Draw();
	}

	// 時計画像1 (後から拡大し、離すと後から戻る)
	if (sprClock_[0]) {
		sprClock_[0]->SetPosition(drawPos);
		// currentScale1_ を適用
		sprClock_[0]->SetSize({sprClock_[0]->GetTextureSize().x * currentScale1_, sprClock_[0]->GetTextureSize().y * currentScale1_});
		sprClock_[0]->Draw();
	}

	// 時計画像2 (先に拡大し、離すと先に戻る)
	if (sprClock_[1]) {
		sprClock_[1]->SetPosition(drawPos);
		// currentScale2_ を適用
		sprClock_[1]->SetSize({sprClock_[1]->GetTextureSize().x * currentScale2_, sprClock_[1]->GetTextureSize().y * currentScale2_});
		sprClock_[1]->Draw();
	}

	// 時計画像3 (そのまま変化しない)
	if (sprClock_[2]) {
		sprClock_[2]->SetPosition(drawPos);
		// currentScale_ (1.0f固定) を適用
		sprClock_[2]->SetSize({sprClock_[2]->GetTextureSize().x * currentScale_, sprClock_[2]->GetTextureSize().y * currentScale_});
		sprClock_[2]->Draw();
	}

	// ギヤ
	for (int i = 0; i < kGearNum; i++) {
		if (gears_[i].size <= 0)
			continue;
		auto* s = sprGear_[gears_[i].textureIndex];
		if (s) {
			s->SetPosition({gears_[i].position.x + shakeOffset_.x, gears_[i].position.y + shakeOffset_.y});
			s->SetRotation(gears_[i].angle);
			s->SetSize({gears_[i].size, gears_[i].size});
			s->Draw();
		}
	}

	// 針の描画
	if (sprHandHour_) {
		sprHandHour_->SetPosition(drawPos);
		sprHandHour_->SetRotation(hourAngle_);
		sprHandHour_->SetSize({sprHandHour_->GetTextureSize().x * currentScale_, sprHandHour_->GetTextureSize().y * currentScale_});
		sprHandHour_->Draw();
	}
	if (sprHandMin_) {
		sprHandMin_->SetPosition(drawPos);
		sprHandMin_->SetRotation(minAngle_);
		sprHandMin_->SetSize({sprHandMin_->GetTextureSize().x * currentScale_, sprHandMin_->GetTextureSize().y * currentScale_});
		sprHandMin_->Draw();
	}

	// Space UI 描画（点滅アニメーション）
	int spaceTexIdx = (spaceAnimTimer_ / 30) % 2;
	if (!isRotating_ && sprSpace_[spaceTexIdx]) {
		sprSpace_[spaceTexIdx]->SetPosition({1496.0f, 544.0f});
		sprSpace_[spaceTexIdx]->Draw();
	}

	// --- 欠片の描画 ---
	for (int i = 0; i < kPieceNum; i++) {
		if (pieces_[i].isDisplay && pieces_[i].sprite) {
			Sprite* s = pieces_[i].sprite;

			s->SetPosition(pieces_[i].position);
			s->SetRotation(pieces_[i].angle);
			float lifeRatio = pieces_[i].life / pieces_[i].maxLife;
			s->SetSize({pieces_[i].width * lifeRatio, pieces_[i].height * lifeRatio});
			s->Draw();
		}
	}

	// --- 光の描画 ---
	for (int i = 0; i < kSparkleNum; i++) {
		if (sparkles_[i].isDisplay && sparkles_[i].sprite) {
			Sprite* s = sparkles_[i].sprite;
			s->SetPosition(sparkles_[i].position);
			s->SetSize({32.0f * sparkles_[i].alpha, 32.0f * sparkles_[i].alpha});
			s->Draw();
		}
	}

	Sprite::PostDraw();
}