#include "Fade.h"

using namespace KamataEngine;

void Fade::Initialize() {
	// エンジン内蔵のデフォルトホワイトテクスチャ(0)を使用して生成
	uint32_t textureHandle = 0;
	sprite_ = Sprite::Create(textureHandle, {0.0f, 0.0f});

	if (sprite_) {
		// 画面全体（1280x720）を覆うサイズに設定
		sprite_->SetSize(Vector2(1280.0f, 720.0f));
		// 初期状態は黒・完全透明にしておく
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	status_ = Status::None;
	counter_ = 0.0f;
	duration_ = 0.0f;
}

void Fade::Update() {
	// フェード中でなければ何もしない
	if (status_ == Status::None) {
		return;
	}

	// 1フレーム分の時間を進める（1秒を60フレームとして計算）
	counter_ += 1.0f / 60.0f;

	// 目標時間に達したらフェード終了
	if (counter_ >= duration_) {
		counter_ = duration_;

		// 終了時の最終色を確定させて状態をNoneに戻す
		if (sprite_) {
			if (status_ == Fade::Status::FadeIn) {
				sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f)); // 完全に透明
			} else if (status_ == Fade::Status::FadeOut) {
				sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f)); // 完全に真っ黒
			}
		}
		status_ = Status::None;
		return;
	}

	// 現在の進行度割合（0.0f ～ 1.0f）を計算
	float rate = counter_ / duration_;

	if (sprite_) {
		if (status_ == Fade::Status::FadeIn) {
			// フェードイン：1.0（真っ黒）から 0.0（透明）へ
			float alpha = 1.0f - rate;
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, alpha));
		} else if (status_ == Fade::Status::FadeOut) {
			// フェードアウト：0.0（透明）から 1.0（真っ黒）へ
			float alpha = rate;
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, alpha));
		}
	}
}

void Fade::Draw() {
	if (sprite_) {
		// KamataEngine必須の前後処理を挟む
		Sprite::PreDraw();

		sprite_->Draw();

		Sprite::PostDraw();
	}
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	// 0除算を防ぐため最低値を保証
	duration_ = (duration <= 0.0f) ? 0.01f : duration;
	counter_ = 0.0f;

	// 開始時のアルファ値を設定
	if (sprite_) {
		if (status_ == Status::FadeIn) {
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f)); // 真っ黒からスタート
		} else if (status_ == Status::FadeOut) {
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f)); // 透明からスタート
		}
	}
}