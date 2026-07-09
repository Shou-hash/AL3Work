#pragma once
#include <KamataEngine.h>

/// <summary>
/// フェード
/// </summary>
class Fade {
public:
	// フェードの状態
	enum class Status {
		None,    // 非表示・通常状態
		FadeIn,  // フェードイン中（暗 -> 明：アルファ値減少）
		FadeOut, // フェードアウト中（明 -> 暗：アルファ値増加）
	};

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// フェード処理の開始
	/// </summary>
	/// <param name="status">フェードインかフェードアウトか</param>
	/// <param name="duration">フェードにかける時間（秒）</param>
	void Start(Status status, float duration);

	/// <summary>
	/// フェードが終了しているかチェック
	/// </summary>
	bool IsFinished() const { return status_ == Status::None; }

private:
	// フェードの中核となる黒スプライト（ポインタ）
	KamataEngine::Sprite* sprite_ = nullptr;

	// 現在のフェードステータス
	Status status_ = Status::None;

	// 経過時間カウント用タイマー
	float counter_ = 0.0f;

	// フェードの総所要時間
	float duration_ = 0.0f;
};