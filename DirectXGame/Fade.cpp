#include "Fade.h"
#include <algorithm> // std::clamp を使用するために追加

using namespace KamataEngine;

void Fade::Initialize() {
	uint32_t textureHandle = 0;
	sprite_ = Sprite::Create(textureHandle, {0.0f, 0.0f});

	if (sprite_) {
		sprite_->SetSize(Vector2(1280.0f, 720.0f));
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	status_ = Status::None;
	counter_ = 0.0f;
	duration_ = 0.0f;
}

void Fade::Update() {
	if (status_ == Status::None) {
		return;
	}

	// 1フレーム分の時間を進め、duration_ を超えないようにクランプ
	counter_ += 1.0f / 60.0f;
	counter_ = std::clamp(counter_, 0.0f, duration_);

	// 現在の進行度割合（0.0f ～ 1.0f）を計算
	float rate = counter_ / duration_;
	float alpha = 0.0f;

	if (status_ == Fade::Status::FadeIn) {
		alpha = 1.0f - rate; // 1.0 から 0.0 へ
	} else if (status_ == Fade::Status::FadeOut) {
		alpha = rate; // 0.0 から 1.0 へ
	}

	if (sprite_) {
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, alpha));
	}

	// 目標時間に達したら状態を終了する
	if (counter_ >= duration_) {
		status_ = Status::None;
	}
}

void Fade::Draw() {
	if (sprite_) {
		Sprite::PreDraw();
		sprite_->Draw();
		Sprite::PostDraw();
	}
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = (duration <= 0.0f) ? 0.01f : duration;
	counter_ = 0.0f;

	if (sprite_) {
		if (status_ == Status::FadeIn) {
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		} else if (status_ == Status::FadeOut) {
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}