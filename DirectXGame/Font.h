#pragma once
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <vector>

// ★ クラスの前方宣言
class LightManager;

/// <summary>
/// フォント演出・コンボ・UI表示専用クラス
/// </summary>
class Font {
public:
	Font() = default;
	~Font();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="camera">カメラポインタ</param>
	void Initialize(KamataEngine::Camera* camera);

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="isCameraPerformance">カメラ演出中かどうかのフラグ（演出中はタイマー減算を停止）</param>
	void Update(bool isCameraPerformance = false);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// ★ 追加: 処刑演出UI（HitIt, PushE）の描画処理
	/// </summary>
	void DrawExecutionUI();

	/// <summary>
	/// ★ 追加: ポーズ画面・UI（Pause, ESC）の描画処理
	/// </summary>
	void DrawPauseUI();

	/// <summary>
	/// ★ 追加: ライティングマネージャの適用
	/// </summary>
	/// <param name="lightManager">ライトマネージャ</param>
	void ApplyLightManager(const LightManager& lightManager);

	/// <summary>
	/// コンボ数の加算（敵に攻撃が当たった際に呼び出す）
	/// </summary>
	/// <param name="amount">加算数（デフォルト: 1）</param>
	void AddCombo(int amount = 1);

	/// <summary>
	/// コンボのリセット
	/// </summary>
	void ResetCombo();

	/// <summary>
	/// 現在のコンボ数を取得
	/// </summary>
	int GetComboCount() const { return comboCount_; }

	/// <summary>
	/// ★ 追加: 処刑演出UI（HitIt, PushE）の表示状態設定
	/// </summary>
	/// <param name="visible">表示フラグ</param>
	void SetExecutionUIVisible(bool visible);

	/// <summary>
	/// ★ 追加: EndScene画面への遷移リクエスト状態を取得
	/// </summary>
	bool IsEndSceneRequested() const { return isEndSceneRequested_; }

	/// <summary>
	/// ★ 追加: ポーズ画面・UI表示状態の設定
	/// </summary>
	void SetPauseUIVisible(bool visible) { isPauseUIVisible_ = visible; }

private:
	KamataEngine::Camera* camera_ = nullptr;

	// モデルポインタ
	KamataEngine::Model* modelCombo_ = nullptr;           // "Combo" OBJモデル
	std::array<KamataEngine::Model*, 10> modelNumbers_{}; // "0" ～ "9" OBJモデル配列

	// ★ 追加: 処刑演出用UIモデル（HitIt, PushE）
	KamataEngine::Model* modelHitIt_ = nullptr;
	KamataEngine::Model* modelPushE_ = nullptr;

	// ★ 追加: ポーズ・ESC UI用モデル
	KamataEngine::Model* modelPause_ = nullptr;
	KamataEngine::Model* modelEsc_ = nullptr;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformCombo_;                // Comboロゴ用
	std::vector<KamataEngine::WorldTransform*> worldTransformDigits_; // ポインタ配列にしてコピー不可エラーを防止

	// ★ 追加: 処刑演出用UIワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformHitIt_;
	KamataEngine::WorldTransform worldTransformPushE_;

	// ★ 追加: ポーズ・ESC UI用ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformPause_;
	KamataEngine::WorldTransform worldTransformEsc_;

	// コンボ管理データ
	int comboCount_ = 0;                           // 現在のコンボ数
	float comboTimer_ = 0.0f;                      // コンボ維持タイマー
	static constexpr float kComboLimitTime = 2.5f; // コンボが途切れるまでの秒数（何秒間貯まっていないと消えるか）

	// ★ 追加: 出現・消去演出用データ
	float appearTimer_ = 0.0f;                         // 出現演出タイマー
	static constexpr float kAppearDuration = 0.25f;    // 出現演出の長さ（秒）
	static constexpr float kDisappearDuration = 0.25f; // 消滅演出の長さ（秒）

	// ★ 追加: コンボ加算ヒット演出用データ
	float hitAnimTimer_ = 0.0f;                      // ヒット演出タイマー
	static constexpr float kHitAnimDuration = 0.20f; // ヒット演出の長さ（秒）

	// ★ 追加: 処刑演出UI（HitIt, PushE）管理データ
	bool isExecutionUIVisible_ = false;                           // 処刑演出UI表示フラグ
	float executionUIAppearTimer_ = 0.0f;                         // 処刑演出UI出現タイマー
	float executionUIDisappearTimer_ = 0.0f;                      // 処刑演出UI消滅タイマー
	static constexpr float kExecutionUIAppearDuration = 0.25f;    // 処刑演出UI出現演出の長さ（秒）
	static constexpr float kExecutionUIDisappearDuration = 0.25f; // 処刑演出UI消滅演出の長さ（秒）

	// ★ 追加: ポーズ・ESC UIの表示フラグとEndScene遷移要求フラグ
	bool isPauseUIVisible_ = true;
	bool isEndSceneRequested_ = false;

	// 表示設定（カメラ追従用の相対座標）
	KamataEngine::Vector3 displayOffset_ = {-8.0f, 2.0f, 12.0f}; // 画面左上付近の配置オフセット

	// ★ 追加: 処刑演出用UIモデルのカメラ追従相対座標オフセット
	KamataEngine::Vector3 displayOffsetHitIt_ = {3.5f, 2.0f, 9.0f}; // 画面右側上部配置オフセット
	KamataEngine::Vector3 displayOffsetPushE_ = {3.5f, 0.8f, 9.0f}; // 画面右側下部配置オフセット

	// ★ 修正: ポーズ・ESC UIモデルのカメラ追従相対座標オフセット（位置調整）
	KamataEngine::Vector3 displayOffsetPause_ = {6.2f, 3.2f, 10.0f}; // 画面右上配置オフセット
	KamataEngine::Vector3 displayOffsetEsc_ = {6.2f, 2.5f, 10.0f};   // Pauseモデルのちょっと下配置オフセット
};