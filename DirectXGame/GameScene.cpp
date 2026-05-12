#include "GameScene.h"
#include "Easeing.h"
#include <cmath>
#include <ctime>

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

KamataEngine::Vector2 GameScene::GetZoomPos(float x, float y) { return {x, y}; }

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
	for (int i = 0; i < 3; i++)
		delete sprBg_[i];
	for (int i = 0; i < 3; i++)
		delete sprClock_[i];
	delete sprHandHour_;
	delete sprHandMin_;
	for (int i = 0; i < 4; i++)
		delete sprPiece_[i];
	for (int i = 0; i < 11; i++)
		delete sprGear_[i];
	delete sprSparkle_;
	// 追加: 太陽と月のメモリ解放
	delete sprSun_;
	delete sprMoon_;
}

void GameScene::Initialize() {
	std::srand((unsigned int)std::time(nullptr));
	camera_.Initialize();

	// 背景・時計盤・針・パーティクル・ギヤの読み込み（元のコードと同じなので省略せずそのまま）
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
		sprHandHour_->SetAnchorPoint({0.5f, 0.5f});
	}
	texHandMin_ = TextureManager::Load("./Resource/Clock/Hand/minHand.png");
	if (texHandMin_) {
		sprHandMin_ = Sprite::Create(texHandMin_, {0, 0});
		sprHandMin_->SetAnchorPoint({0.5f, 0.5f});
	}

	for (int i = 0; i < 4; i++) {
		texPiece_[i] = TextureManager::Load("./Resource/Clock/Piece/piece" + std::to_string(i + 1) + ".png");
		if (texPiece_[i]) {
			sprPiece_[i] = Sprite::Create(texPiece_[i], {0, 0});
			sprPiece_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	texSparkle_ = TextureManager::Load("./Resource/Clock/Light/particle.png");
	if (texSparkle_) {
		sprSparkle_ = Sprite::Create(texSparkle_, {0, 0});
		sprSparkle_->SetAnchorPoint({0.5f, 0.5f});
	}

	for (int i = 0; i < 11; i++) {
		texGear_[i] = TextureManager::Load("./Resource/Gear/gear" + std::to_string(i + 1) + ".png");
		if (texGear_[i]) {
			sprGear_[i] = Sprite::Create(texGear_[i], {0, 0});
			sprGear_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	// --- 追加: 太陽と月の読み込み ---
	// ※画像ファイルのパスはご自身の環境に合わせて変更してください
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

	for (int i = 0; i < kGearNum; i++) {
		gears_[i].position = kGearInitDatas[i].position;
		gears_[i].radius = kGearInitDatas[i].radius;
		gears_[i].rotateSpeed = kGearInitDatas[i].rotateSpeed;
		gears_[i].size = kGearInitDatas[i].grSize;
		gears_[i].textureIndex = kGearInitDatas[i].textureIndex;
		gears_[i].angle = 0.0f;
	}

	for (auto& p : pieces_)
		p.isDisplay = false;
	for (auto& s : sparkles_)
		s.isDisplay = false;
}

void GameScene::Update() {
	Input* input = Input::GetInstance();

	// --- 色変え（昼/夜 切り替え）処理 ---
	if (input->TriggerKey(DIK_C)) {
		isSunActive_ = !isSunActive_;
	}

	if (!isSunActive_) {
		colorLerpTimer_ += kColorChangeSpeed_;
		if (colorLerpTimer_ > 1.0f)
			colorLerpTimer_ = 1.0f;
	} else {
		colorLerpTimer_ -= kColorChangeSpeed_;
		if (colorLerpTimer_ < 0.0f)
			colorLerpTimer_ = 0.0f;
	}

	// --- 時計回転ロジック（逆回転と時針連動を追加） ---
	if (!isRotating_) {
		bool isSpacePressed = input->TriggerKey(DIK_SPACE);
		intervalTimer_++;

		// 一定時間経過するか、スペースキーが押されたら回転開始
		if (intervalTimer_ > 120 || isSpacePressed) {
			isRotating_ = true;
			easeTimer_ = 0.0f;
			minStart_ = minAngle_;
			hourStart_ = hourAngle_;

			// 追加: 分針が30度進むと、時針は2.5度進む (30度 / 12)
			float moveAngleMin = 0.523f;   // 約30度
			float moveAngleHour = 0.0436f; // 約2.5度

			// スペースキーなら逆回転、それ以外は正回転
			if (isSpacePressed) {
				minTarget_ = minAngle_ - moveAngleMin;
				hourTarget_ = hourAngle_ - moveAngleHour;
			} else {
				minTarget_ = minAngle_ + moveAngleMin;
				hourTarget_ = hourAngle_ + moveAngleHour;
			}

			shakeTimer_ = 30.0f;
			intervalTimer_ = 0;

			// 欠片の放出
			for (auto& p : pieces_) {
				p.isDisplay = true;
				p.position = clockPos_;
				// 逆回転時は飛び散り方を変えたい場合はここで調整可能です
				p.velocity = {Random(-5, 5), Random(-10, -2)};
				p.angle = Random(0, 6.28f);
				p.angularVelocity = Random(-0.1f, 0.1f);
				p.textureIndex = std::rand() % 4;
				p.width = Random(16.0f, 48.0f);
				p.height = p.width;
			}

			// 光(星)の放出
			for (int i = 0; i < 15; i++) {
				for (auto& s : sparkles_) {
					if (!s.isDisplay) {
						s.isDisplay = true;
						s.position = {clockPos_.x + Random(-150, 150), clockPos_.y + Random(-150, 150)};
						s.velocity = {Random(-2, 2), Random(-5, -1)};
						s.alpha = 1.0f;
						s.lifeSpeed = Random(0.01f, 0.03f);
						break;
					}
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
			// 追加: 時針も分針と一緒にイージングをかけて回す
			float easeVal = EaseOutQuart(easeTimer_);
			minAngle_ = minStart_ + (minTarget_ - minStart_) * easeVal;
			hourAngle_ = hourStart_ + (hourTarget_ - hourStart_) * easeVal;
		}
	}

	// シェイク更新
	if (shakeTimer_ > 0) {
		shakeTimer_ -= 1.0f;
		shakeOffset_ = {Random(-5, 5), Random(-5, 5)};
	} else {
		shakeOffset_ = {0, 0};
	}

	// ギヤ回転
	for (auto& g : gears_) {
		g.angle += g.rotateSpeed;
	}

	// 欠片の物理更新
	for (auto& p : pieces_) {
		if (p.isDisplay) {
			p.position.x += p.velocity.x;
			p.position.y += p.velocity.y;
			p.velocity.y += 0.5f;
			p.angle += p.angularVelocity;
			if (p.position.y > 1080) {
				p.isDisplay = false;
			}
		}
	}

	// 光の更新
	for (auto& s : sparkles_) {
		if (s.isDisplay) {
			s.position.x += s.velocity.x;
			s.position.y += s.velocity.y;
			s.alpha -= s.lifeSpeed;
			if (s.alpha <= 0.0f) {
				s.isDisplay = false;
			}
		}
	}
}

void GameScene::Draw() {
	Sprite::PreDraw();

	Vector2 drawPos = {clockPos_.x + shakeOffset_.x, clockPos_.y + shakeOffset_.y};

	// 背景
	if (sprBg_[0])
		sprBg_[0]->Draw();
	if (sprBg_[1] && colorLerpTimer_ > 0.0f) {
		// ※エンジンによってはアルファブレンド関数が必要です
		// sprBg_[1]->SetColor({1.0f, 1.0f, 1.0f, colorLerpTimer_});
		sprBg_[1]->Draw();
	}

	// --- 追加: 太陽と月の描画 ---
	// colorLerpTimer_ に応じて高さを変えるなどの演出を加えています
	if (sprSun_) {
		// 昼(0.0)のときは画面上部、夜(1.0)のときは画面下部へ沈む
		float sunY = 200.0f + (colorLerpTimer_ * 500.0f);
		sprSun_->SetPosition({300.0f, sunY});
		sprSun_->Draw();
	}
	if (sprMoon_) {
		// 夜(1.0)のときは画面上部、昼(0.0)のときは画面下部へ沈む
		float moonY = 700.0f - (colorLerpTimer_ * 500.0f);
		sprMoon_->SetPosition({980.0f, moonY});
		sprMoon_->Draw();
	}

	// 時計本体
	for (int i = 2; i >= 0; i--) {
		if (sprClock_[i]) {
			sprClock_[i]->SetPosition(drawPos);
			sprClock_[i]->Draw();
		}
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

	// 針の描画 (時針と分針)
	if (sprHandHour_) {
		sprHandHour_->SetPosition(drawPos);
		sprHandHour_->SetRotation(hourAngle_); // 計算された時針の角度を適用
		sprHandHour_->Draw();
	}
	if (sprHandMin_) {
		sprHandMin_->SetPosition(drawPos);
		sprHandMin_->SetRotation(minAngle_);
		sprHandMin_->Draw();
	}

	// 欠片の描画
	for (auto& p : pieces_) {
		if (p.isDisplay) {
			auto* s = sprPiece_[p.textureIndex];
			if (s) {
				s->SetPosition(p.position);
				s->SetRotation(p.angle);
				s->SetSize({p.width, p.height});
				s->Draw();
			}
		}
	}

	// 光の描画
	for (auto& s : sparkles_) {
		if (s.isDisplay && sprSparkle_) {
			sprSparkle_->SetPosition(s.position);
			sprSparkle_->SetSize({32.0f * s.alpha, 32.0f * s.alpha});
			sprSparkle_->Draw();
		}
	}

	Sprite::PostDraw();
}