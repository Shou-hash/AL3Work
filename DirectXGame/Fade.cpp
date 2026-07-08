#include "Fade.h"

using namespace KamataEngine;

void Fade::Initialize() {
	// 白または黒の1x1ピクセルなどのテクスチャを読み込み、画面全体を覆うスプライトを作成する
	// プロジェクト環境に合わせたテクスチャハンドルを指定
	uint32_t textureHandle = TextureManager::Load("white1x1.png");

	// スプライトの生成と初期位置・サイズの設定（画面全体 1280x720 を覆う）
	sprite_ = Sprite::Create(textureHandle, {0.0f, 0.0f});
	if (sprite_) {
		sprite_->SetSize(Vector2{1280, 720});
		sprite_->SetColor(Vector4{0, 0, 0, 1});
	}

	status_ = Status::None;
	counter_ = 0.0f;
	duration_ = 0.0f;
}

void Fade::Update() {

	switch (status_) {
	case Fade::Status::None:
		break;

	case Fade::Status::FadeIn:
		// 1フレーム分の時間を進める (60FPS想定)
		counter_ += 1.0f / 60.0f;

		// 終了判定
		if (counter_ >= duration_) {
			counter_ = duration_;
			status_ = Status::None; // フェードイン完了
		}

		// アルファ値の計算 (1.0 から 0.0 へ減少：画面が徐々に明るくなる)
		if (sprite_) {
			float rate = counter_ / duration_;
			float alpha = 1.0f - rate;
			sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha});
		}
		break;

	case Fade::Status::FadeOut:
		// 1フレーム分の時間を進める (60FPS想定)
		counter_ += 1.0f / 60.0f;

		// 終了判定
		if (counter_ >= duration_) {
			counter_ = duration_;
			status_ = Status::None; // フェードアウト完了
		}

		// アルファ値の計算 (0.0 から 1.0 へ増加：画面が徐々に暗くなる)
		if (sprite_) {
			float rate = counter_ / duration_;
			float alpha = rate;
			sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha});
		}
		break;

	default:
		break;
	}
}

void Fade::Draw() {
	// フェード状態がNoneではないとき、または画面が暗転しているとき（アルファ値が0より大きいとき）に描画
	if (status_ != Status::None || (sprite_ && sprite_->GetColor().w > 0.0f)) {
		if (sprite_) {
			sprite_->Draw();
		}
	}
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f; // タイマーをリセット

	// 開始時の初期アルファ値を設定
	if (sprite_) {
		float initialAlpha = (status_ == Status::FadeIn) ? 1.0f : 0.0f;
		sprite_->SetColor({0.0f, 0.0f, 0.0f, initialAlpha});
	}
}

bool Fade::IsFinished() const {
	// ステータスが None に戻っていればフェード処理が完了しているとみなす
	return status_ == Status::None;
}