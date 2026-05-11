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
}

void GameScene::Initialize() {
	std::srand((unsigned int)std::time(nullptr));
	camera_.Initialize();

	// 背景の読み込み
	for (int i = 0; i < 3; i++) {
		texBg_[i] = TextureManager::Load("./Resource/Clock/Bg/bg" + std::to_string(i + 1) + ".png");
		if (texBg_[i])
			sprBg_[i] = Sprite::Create(texBg_[i], {0, 0});
	}

	// 時計盤
	for (int i = 0; i < 3; i++) {
		texClock_[i] = TextureManager::Load("./Resource/Clock/clock" + std::to_string(i + 1) + ".png");
		if (texClock_[i]) {
			sprClock_[i] = Sprite::Create(texClock_[i], {0, 0});
			sprClock_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	// 針
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

	// 欠片
	for (int i = 0; i < 4; i++) {
		texPiece_[i] = TextureManager::Load("./Resource/Clock/Piece/piece" + std::to_string(i + 1) + ".png");
		if (texPiece_[i]) {
			sprPiece_[i] = Sprite::Create(texPiece_[i], {0, 0});
			sprPiece_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	// 光パーティクル
	texSparkle_ = TextureManager::Load("./Resource/Clock/Light/particle.png");
	if (texSparkle_) {
		sprSparkle_ = Sprite::Create(texSparkle_, {0, 0});
		sprSparkle_->SetAnchorPoint({0.5f, 0.5f});
	}

	// ギヤ
	for (int i = 0; i < 11; i++) {
		texGear_[i] = TextureManager::Load("./Resource/Gear/gear" + std::to_string(i + 1) + ".png");
		if (texGear_[i]) {
			sprGear_[i] = Sprite::Create(texGear_[i], {0, 0});
			sprGear_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	// --- 全ギヤの初期配置設定 ---
	for (int i = 0; i < kGearNum; i++) {
		gears_[i].position = kGearInitDatas[i].position;
		gears_[i].radius = kGearInitDatas[i].radius;
		gears_[i].rotateSpeed = kGearInitDatas[i].rotateSpeed;
		gears_[i].size = kGearInitDatas[i].grSize;
		gears_[i].textureIndex = kGearInitDatas[i].textureIndex;
		gears_[i].angle = 0.0f;
	}

	// --- 欠片・光の初期化 ---
	for (auto& p : pieces_)
		p.isDisplay = false;
	for (auto& s : sparkles_)
		s.isDisplay = false;
}

void GameScene::Update() {
	Input* input = Input::GetInstance();

	// --- 色変え（昼/夜 切り替え）処理 ---
	if (input->TriggerKey(DIK_C)) {
		isSunActive_ = !isSunActive_; // Cキーで色味の切り替えテスト
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

	// --- 時計回転ロジック ---
	if (!isRotating_) {
		intervalTimer_++;
		if (intervalTimer_ > 120 || input->TriggerKey(DIK_SPACE)) {
			isRotating_ = true;
			easeTimer_ = 0.0f;
			minStart_ = minAngle_;
			minTarget_ = minAngle_ + 0.523f; // 30度
			shakeTimer_ = 30.0f;
			intervalTimer_ = 0;

			// 欠片の放出
			for (auto& p : pieces_) {
				p.isDisplay = true;
				p.position = clockPos_;
				p.velocity = {Random(-5, 5), Random(-10, -2)};
				p.angle = Random(0, 6.28f);
				p.angularVelocity = Random(-0.1f, 0.1f);
				p.textureIndex = std::rand() % 4;
				p.width = Random(16.0f, 48.0f);
				p.height = p.width;
			}

			// 光(星)の放出
			for (int i = 0; i < 15; i++) { // 1度に15個放出
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
		} else {
			minAngle_ = minStart_ + (minTarget_ - minStart_) * EaseOutQuart(easeTimer_);
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

	// 欠片(Piece)の物理更新
	for (auto& p : pieces_) {
		if (p.isDisplay) {
			p.position.x += p.velocity.x;
			p.position.y += p.velocity.y;
			p.velocity.y += 0.5f;         // 重力
			p.angle += p.angularVelocity; // 回転
			if (p.position.y > 1080) {
				p.isDisplay = false;
			}
		}
	}

	// 光(Sparkle)の更新
	for (auto& s : sparkles_) {
		if (s.isDisplay) {
			s.position.x += s.velocity.x;
			s.position.y += s.velocity.y;
			s.alpha -= s.lifeSpeed; // 透明度を下げる
			if (s.alpha <= 0.0f) {
				s.isDisplay = false;
			}
		}
	}
}

void GameScene::Draw() {
	Sprite::PreDraw();

	Vector2 drawPos = {clockPos_.x + shakeOffset_.x, clockPos_.y + shakeOffset_.y};

	// 2D背景の色変え (KamataEngine側での色の合成手法に準拠)
	// 例として、colorLerpTimer_に応じて全体の色調（アルファなど）を制御できます
	if (sprBg_[0])
		sprBg_[0]->Draw(); // 昼背景
	if (sprBg_[1] && colorLerpTimer_ > 0.0f) {
		// 夜背景をアルファブレンドで乗せる（エンジン仕様に合わせてSetColor等でアルファを指定）
		// sprBg_[1]->SetColor({1.0f, 1.0f, 1.0f, colorLerpTimer_});
		sprBg_[1]->Draw();
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
		if (gears_[i].size <= 0) {
			continue;
		}
		auto* s = sprGear_[gears_[i].textureIndex];
		if (s) {
			s->SetPosition({gears_[i].position.x + shakeOffset_.x, gears_[i].position.y + shakeOffset_.y});
			s->SetRotation(gears_[i].angle);
			s->SetSize({gears_[i].size, gears_[i].size});
			s->Draw();
		}
	}

	// 針
	if (sprHandHour_) {
		sprHandHour_->SetPosition(drawPos);
		sprHandHour_->SetRotation(hourAngle_);
		sprHandHour_->Draw();
	}
	if (sprHandMin_) {
		sprHandMin_->SetPosition(drawPos);
		sprHandMin_->SetRotation(minAngle_);
		sprHandMin_->Draw();
	}

	// --- 欠片(Piece)の描画 ---
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

	// --- 光(Sparkle)の描画 ---
	for (auto& s : sparkles_) {
		if (s.isDisplay && sprSparkle_) {
			sprSparkle_->SetPosition(s.position);
			// エンジンに応じてアルファ値を反映: sprSparkle_->SetColor({1.0f, 1.0f, 1.0f, s.alpha});
			sprSparkle_->SetSize({32.0f * s.alpha, 32.0f * s.alpha}); // 透明度に合わせてサイズも縮小する擬似表現
			sprSparkle_->Draw();
		}
	}

	Sprite::PostDraw();
}

float GameScene::Random(float min, float max) { return min + (float)std::rand() / RAND_MAX * (max - min); }

KamataEngine::Vector2 GameScene::GetZoomPos(float x, float y) {
	// 将来的なズーム実装用ヘルパー
	return {x, y};
}