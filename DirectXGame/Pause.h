#pragma once
#include "KamataEngine.h"
#include <3d/WorldTransform.h>

// ★ クラスの前方宣言
class LightManager;

/// <summary>
/// ESCポーズ・看板UI制御専用クラス
/// </summary>
class Pause {
public:
	// ★ メニュー項目の定義
	enum class MenuType {
		Return, // ゲームに戻る
		Retry,  // リトライ
		Title,  // タイトルに戻る
		Exit,   // ゲーム終了
		Count   // 項目の総数
	};

	// ★ ポーズ画面の表示モード（ゲーム中 / セレクト画面中）
	enum class Mode {
		Game,  // 4項目 (Return, Retry, Title, Exit)
		Select // 2項目 (Title, Exit)
	};

	Pause() = default;
	~Pause();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="camera">カメラポインタ</param>
	void Initialize(KamataEngine::Camera* camera);

	/// <summary>
	/// 更新処理（ESCキー入力判定、メニュー選択処理、看板モデルの降り/上がりアニメーション）
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理（看板モデルおよびメニュー選択モデルの描画）
	/// </summary>
	void Draw();

	/// <summary>
	/// ★ ライティングマネージャの適用
	/// </summary>
	/// <param name="lightManager">ライトマネージャ</param>
	void ApplyLightManager(const LightManager& lightManager);

	/// <summary>
	/// ★ ポーズ表示モードの設定（ゲーム中かセレクト画面か）
	/// </summary>
	/// <param name="mode">ポーズモード</param>
	void SetMode(Mode mode);

	/// <summary>
	/// ★ ポーズ中（またはポーズアニメーション中）かどうかを取得
	/// </summary>
	bool IsPaused() const { return isPaused_; }

	/// <summary>
	/// ★ メニュー項目が確定選択されたかどうかを取得
	/// </summary>
	bool IsMenuSelected() const { return isMenuSelected_; }

	/// <summary>
	/// ★ 確定選択されたメニュー項目を取得
	/// </summary>
	MenuType GetSelectedMenu() const { return selectedMenu_; }

	/// <summary>
	/// ★ 確定選択フラグをリセット
	/// </summary>
	void ResetMenuSelection() { isMenuSelected_ = false; }

	/// <summary>
	/// ★ 看板を閉じる（ポーズ解除）
	/// </summary>
	void Close() { isTargetPaused_ = false; }

	/// <summary>
	/// ★ 看板モデルのScaleを設定
	/// </summary>
	/// <param name="scale">スケール</param>
	void SetScale(const KamataEngine::Vector3& scale) { worldTransform_.scale_ = scale; }

	/// <summary>
	/// ★ 看板のカメラ相対オフセット位置（Z軸距離・展開高さ等）を設定
	/// </summary>
	/// <param name="hiddenOffset">画面上方隠れ位置オフセット</param>
	/// <param name="visibleOffset">画面中央表示位置オフセット</param>
	void SetOffsets(const KamataEngine::Vector3& hiddenOffset, const KamataEngine::Vector3& visibleOffset) {
		hiddenOffset_ = hiddenOffset;
		visibleOffset_ = visibleOffset;
	}

	/// <summary>
	/// ★ 移動解説モデル（MoveExplanation）の個別パラメータ（Scale XYZ倍率, 相対オフセット XYZ）を設定
	/// </summary>
	void SetMoveExplanationParam(const KamataEngine::Vector3& scaleMultiplier, const KamataEngine::Vector3& offset) {
		scaleMultiplierMove_ = scaleMultiplier;
		offsetMove_ = offset;
	}

	/// <summary>
	/// ★ 攻撃解説モデル（AttackExplanation）の個別パラメータ（Scale XYZ倍率, 相対オフセット XYZ）を設定
	/// </summary>
	void SetAttackExplanationParam(const KamataEngine::Vector3& scaleMultiplier, const KamataEngine::Vector3& offset) {
		scaleMultiplierAttack_ = scaleMultiplier;
		offsetAttack_ = offset;
	}

	/// <summary>
	/// ★ ダッシュ解説モデル（DashExplanation）の個別パラメータ（Scale XYZ倍率, 相対オフセット XYZ）を設定
	/// </summary>
	void SetDashExplanationParam(const KamataEngine::Vector3& scaleMultiplier, const KamataEngine::Vector3& offset) {
		scaleMultiplierDash_ = scaleMultiplier;
		offsetDash_ = offset;
	}

private:
	KamataEngine::Camera* camera_ = nullptr;

	// ★ 看板（PictureFrame）モデルポインタとワールドトランスフォーム
	KamataEngine::Model* modelPictureFrame_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	// ★ 操作説明用モデルポインタ（MoveExplanation, AttackExplanation, DashExplanation）
	KamataEngine::Model* modelMoveExplanation_ = nullptr;
	KamataEngine::Model* modelAttackExplanation_ = nullptr;
	KamataEngine::Model* modelDashExplanation_ = nullptr;

	// ★ 操作説明モデル用ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformMove_;
	KamataEngine::WorldTransform worldTransformAttack_;
	KamataEngine::WorldTransform worldTransformDash_;

	// ★ メニュー選択モデル用（Return, Retry, Title, Exit）のモデルポインタ配列とワールドトランスフォーム配列
	static inline const int32_t kNumMenus = static_cast<int32_t>(MenuType::Count);
	KamataEngine::Model* menuModels_[kNumMenus] = {nullptr};
	KamataEngine::WorldTransform menuWorldTransforms_[kNumMenus];

	// ★ ポーズモード管理
	Mode mode_ = Mode::Game;

	// ★ メニュー選択管理用データ
	int32_t currentSelectIndex_ = 0;
	MenuType selectedMenu_ = MenuType::Return;
	bool isMenuSelected_ = false;
	float menuAnimationTimers_[kNumMenus] = {0.0f};

	// ★ 画面下に横並び描画するための各メニューモデルの相対オフセットパラメータ
	KamataEngine::Vector3 scaleMultiplierMenu_ = {6.5f, 6.5f, 6.5f}; // メニュー基本拡大率
	float menuPosY_ = -1.05f;                                        // 画面下に配置するためのY相対オフセット
	float menuPosZ_ = -0.05f;                                        // Z相対オフセット

	// ★ 各操作説明モデルの個別Scale倍率（1.5倍に拡大設定）
	KamataEngine::Vector3 scaleMultiplierMove_ = {10.5f, 10.5f, 10.5f};
	KamataEngine::Vector3 scaleMultiplierAttack_ = {10.5f, 10.5f, 10.5f};
	KamataEngine::Vector3 scaleMultiplierDash_ = {10.5f, 10.5f, 10.5f};

	// ★ 各操作説明モデルの個別相対オフセット位置（左上に寄せ、間隔を狭めて配置）
	KamataEngine::Vector3 offsetMove_ = {-0.6f, 1.15f, -0.05f};
	KamataEngine::Vector3 offsetAttack_ = {-0.6f, 0.50f, -0.05f};
	KamataEngine::Vector3 offsetDash_ = {-0.6f, -0.15f, -0.05f};

	// ★ ポーズ状態管理データ
	bool isPaused_ = false;                       // ポーズ中フラグ（ゲーム挙動ストップ用）
	bool isTargetPaused_ = false;                 // 目標ポーズ状態（ESCキーでトグル切り替え）
	float animTimer_ = 0.0f;                      // 降り・上がりアニメーションタイマー
	static constexpr float kAnimDuration = 0.35f; // アニメーション演出時間（秒）

	// ★ 相対オフセット座標設定（カメラ座標基準）
	KamataEngine::Vector3 hiddenOffset_ = {0.0f, 12.0f, 8.0f}; // 画面上方隠れ位置
	KamataEngine::Vector3 visibleOffset_ = {0.0f, 0.0f, 8.0f}; // 画面中央表示位置
};