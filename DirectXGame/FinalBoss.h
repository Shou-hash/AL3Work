#pragma once
#define NOMINMAX
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <algorithm>
#include <array>
#include <vector>

class MapChipField;
class Player;

class FinalBoss : public BaseEnemy {
public:
	static constexpr size_t kNumHands = 7;
	static constexpr size_t kNumCrystals = 22;                  // ★ 22個のクリスタル定数に変更
	static constexpr size_t kMeteorCrystals = kNumCrystals - 1; // ★ 流星群で飛ぶクリスタルの数（21個）
	static constexpr size_t kNumMeteorShower = 20;              // ★ 流星群として降る finalBossCrystal_22 の数

	// ★ ボスの状態定義
	enum class State {
		kStandby,    // 待機状態（プレイヤー接近前・完全非表示）
		kSpawn,      // 出現アニメーション中
		kNormal,     // 通常状態
		kFrenzyInit, // 発狂初期演出中（振動・溜め）
		kFrenzy,     // 発狂待機状態（攻撃インターバル待ち）
		kAttack,     // ★ 流星群（クリスタル落下）攻撃状態
		kDefeat      // ★ 撃破アニメーション中（痙攣 ➔ 崩れ落ちる ➔ 縮小して消える）
	};

	// ★ クリスタルの攻撃状態定義
	enum class CrystalAttackStatus {
		kIdle,       // 通常浮遊状態
		kTargeting,  // 落下目的（予告マーカー）表示中
		kFalling,    // 落下中
		kReturning,  // 元の位置へ復帰中
		kLaunching,  // 空へ発射中
		kSkyFalling, // 空から落下中
		kLanded,     // ステージ上に着地
		kHitFlying   // プレイヤーに打たれてラスボスへ飛行中
	};

	// ★ 各クリスタルの攻撃管理構造体
	struct CrystalAttackInfo {
		CrystalAttackStatus status = CrystalAttackStatus::kIdle;
		KamataEngine::Vector3 targetPosition{};  // 落下目標ワールド座標
		KamataEngine::Vector3 startPosition{};   // 落下開始ワールド座標
		KamataEngine::Vector3 currentWorldPos{}; // 現在のワールド座標
		float timer = 0.0f;                      // タイマー
		float targetingDuration = 0.6f;          // 予告表示時間（秒）
		float fallDuration = 0.35f;              // 落下時間（秒）
		float returnDuration = 0.5f;             // 復帰時間（秒）
	};

	// ★ 流星群用 finalBossCrystal_22 個別データ構造体（バウンド・物理移動対応）
	struct MeteorCrystal {
		KamataEngine::WorldTransform worldTransform{};
		KamataEngine::Vector3 position{};       // 現在の座標
		KamataEngine::Vector3 velocity{};       // 速度ベクトル
		KamataEngine::Vector3 targetPosition{}; // ★ 追加: 着地予定座標（予告マーカー用）
		KamataEngine::Vector3 scale{1.0f, 1.0f, 1.0f};
		float timer = 0.0f;
		float delay = 0.0f;
		int bounceCount = 0; // 現在のバウンド回数
		int maxBounces = 3;  // 最大バウンド回数（3回跳ねる）
		bool active = false;
	};

	// ★ パーティクル用構造体
	struct SpawnParticle {
		KamataEngine::Vector3 position;
		KamataEngine::Vector3 scale;
		KamataEngine::Vector3 velocity;
		KamataEngine::Vector4 color;
		float currentLife = 0.0f;
		float maxLife = 1.0f;
	};

	FinalBoss() = default;
	~FinalBoss() override; // ★ スプライト解放用デストラクタを追加

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="modelBody">本体モデル (finalBoss)</param>
	/// <param name="modelHands">7本の手のモデル配列 (finalBossHand_1 ~ 7)</param>
	/// <param name="modelCrystals">22個のクリスタルモデル配列 (finalBossCrystal_1 ~ 22)</param>
	/// <param name="modelCrystal22">攻撃専用 finalBossCrystal_22</param>
	/// <param name="camera">カメラ</param>
	/// <param name="position">初期配置座標</param>
	void Initialize(
	    KamataEngine::Model* modelBody, const std::array<KamataEngine::Model*, kNumHands>& modelHands, const std::array<KamataEngine::Model*, kNumCrystals>& modelCrystals,
	    KamataEngine::Model* modelCrystal22, // ★ 引数変更 (modelCrystal22)
	    KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update() override;
	void Draw() override;
	void OnCollision(Player* player) override;
	void OnDead() override;
	AABB GetAABB() const override;

	void SetPlayer(Player* player) { player_ = player; }

	// ★ マップチップフィールドの設定（流星群の地形衝突判定用）
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// 出現・発狂アニメーション制御関数
	void StartSpawn();  // ★ 出現アニメーション開始
	void StartFrenzy(); // 発狂アニメーション開始
	State GetState() const { return state_; }

	// ★ 流星群攻撃の開始
	void StartAttack();
	bool IsCrystal22Landed() const { return crystal22Landed_; }
	bool IsCrystal22LightActive() const { return crystal22Landed_ && !crystal22Flying_; }
	const KamataEngine::Vector3& GetCrystal22Position() const { return worldTransformCrystal22_.translation_; }

	// ★ 流星群攻撃中判定および降下中 finalBossCrystal_22 の位置取得アクセサ
	bool IsMeteorShowerActive() const { return state_ == State::kAttack; }

	// ★ 現在アクティブな finalBossCrystal_22 (流星群および攻撃用クリスタル) の座標リストを取得
	std::vector<KamataEngine::Vector3> GetActiveMeteorPositions() const;

	bool HitCrystal22();
	bool IsDefeated() const { return isFinalBossDefeated_; }

	// ★ 撃破アニメーションの進行度（0.0 ~ 1.0）と終了判定（終了したらゴールを表示してゴール追従カメラ演出を開始する）
	float GetDefeatProgress() const { return state_ == State::kDefeat ? (std::min)(defeatTimer_ / kDefeatDuration, 1.0f) : (isDefeatAnimationFinished_ ? 1.0f : 0.0f); }

	bool IsDefeatAnimationFinished() const { return isDefeatAnimationFinished_; }

	// ★ 流星群が降り始めたか（攻撃開始のカメラ演出を終えるタイミングに使う）
	bool IsMeteorShowerStarted() const { return meteorShowerStarted_; }
	int GetFinalBossHp() const { return finalBossHp_; }
	int GetMaxFinalBossHp() const { return maxFinalBossHp_; }
	bool ConsumeFinalBossHit() {
		bool hit = finalBossHitEvent_;
		finalBossHitEvent_ = false;
		return hit;
	}

	// ★ 出現演出の進行度（0.0 ~ 1.0）を返す（ライトの暗転演出用）。待機中は 0.0、出現後は 1.0
	float GetSpawnProgress() const {
		if (state_ == State::kStandby) {
			return 0.0f;
		}
		if (state_ == State::kSpawn) {
			float progress = spawnTimer_ / spawnDuration_;
			return progress > 1.0f ? 1.0f : progress;
		}
		return 1.0f;
	}

	// ★ パーティクル処理用のヘルパー関数
	void EmitSpawnParticles();
	void EmitDefeatParticles(); // ★ 撃破アニメーション用（本体から外側へ飛び散る）
	void UpdateParticles();
	void DrawParticles();

	// ★ 落下目標予告マーカーの描画処理（white1x1.png & Sprite::BlendMode::kAdd 加算ブレンディング）
	void DrawTargetMarkers();

	// ★ 3Dワールド座標から2Dスクリーン座標への変換ヘルパー関数
	KamataEngine::Vector2 WorldToScreen(const KamataEngine::Vector3& worldPos, const KamataEngine::Camera& camera);

	// 描画座標調整用オフセットのアクセサ
	KamataEngine::Vector3& GetDrawOffset() { return drawOffset_; }

	// ImGui調整用アクセサ
	KamataEngine::Vector3& GetBasePosition() { return basePosition_; }
	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	// 行列積の計算ヘルパー関数（Player/BossEnemy同様）
	KamataEngine::Matrix4x4 MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b);

	KamataEngine::Camera* camera_ = nullptr;
	Player* player_ = nullptr;
	MapChipField* mapChipField_ = nullptr; // ★ マップチップフィールド（地形衝突判定用）

	// モデル参照
	KamataEngine::Model* modelBody_ = nullptr;
	std::array<KamataEngine::Model*, kNumHands> modelHands_{};
	std::array<KamataEngine::Model*, kNumCrystals> modelCrystals_{}; // ★ クリスタルモデル配列 (22個)
	KamataEngine::Model* modelCrystal22_ = nullptr;                  // ★ 攻撃専用 finalBossCrystal_22

	// ワールドトランスフォーム
	// worldTransform_ (親 / ルート座標)
	KamataEngine::WorldTransform worldTransformBody_;                               // 本体ローカル
	std::array<KamataEngine::WorldTransform, kNumHands> worldTransformHands_;       // 手のローカル＆合成用
	std::array<KamataEngine::WorldTransform, kNumCrystals> worldTransformCrystals_; // ★ クリスタルのローカル＆合成用 (22個)
	KamataEngine::WorldTransform worldTransformCrystal22_;                          // ★ 攻撃専用クリスタル

	// ★ 描画で使い回すパーティクル用の WorldTransform
	KamataEngine::WorldTransform particleWorldTransform_;

	// 奥行き・背景（パララックス）用変数 (BackgroundBlocksを参考)
	KamataEngine::Vector3 basePosition_{};

	// 見た目（描画位置）のみをずらすための調整用オフセット
	KamataEngine::Vector3 drawOffset_{-33.0f, -7.8f, 0.0f};

	float parallaxFactor_ = 0.35f; // 視差係数（手前よりゆっくり動き、奥深さを演出）

	// アニメーションタイマー
	float animTimer_ = 0.0f;
	State state_ = State::kStandby; // ★ 初期状態を待機中(kStandby)に設定
	float frenzyTimer_ = 0.0f;      // ★ 発狂初期演出用タイマー

	// ★ 流星群攻撃管理用変数
	float frenzyWaitTimer_ = 0.0f;    // 待機状態タイマー
	float frenzyWaitDuration_ = 6.0f; // ★ 流星群攻撃再実行までの待機時間（6.0秒以上）

	bool attackDamageDone_ = false; // ★ 追加: このターンの攻撃でラスボスがダメージを受けたか（攻撃の終了判定用）

	// ★ 攻撃開始・攻撃中の本体アニメーション用
	float attackTimer_ = 0.0f;                          // 攻撃開始からの経過時間
	static constexpr float kAttackIntroDuration = 1.2f; // 攻撃開始の「溜め ➔ 咆哮」アニメーションの長さ（秒）

	// ★ 撃破アニメーション用
	float defeatTimer_ = 0.0f;                     // 撃破からの経過時間
	bool isDefeatAnimationFinished_ = false;       // 撃破アニメーションが終わったか
	static constexpr float kDefeatDuration = 4.0f; // 撃破アニメーションの長さ（秒）
	bool meteorShowerStarted_ = false;             // ★ 追加: 周りのクリスタルが全部飛び立った後に流星群を開始するフラグ

	std::array<CrystalAttackInfo, kNumCrystals> crystalAttackInfos_{}; // 22個のクリスタル個別の攻撃データ
	std::array<MeteorCrystal, kNumMeteorShower> meteorCrystals_{};     // ★ 流星群用 finalBossCrystal_22 配列
	std::array<bool, kMeteorCrystals> crystalHidden_{};
	bool crystal22Landed_ = false;
	bool crystal22Flying_ = false;
	bool isFinalBossDefeated_ = false;
	bool finalBossHitEvent_ = false;
	int crystal22HitCount_ = 0;
	int crystal22HitsRequired_ = 2;
	int finalBossHp_ = 6;
	int maxFinalBossHp_ = 6;
	float crystal22Timer_ = 0.0f;
	float crystal22LandedTimer_ = 0.0f;
	float hitReactionTimer_ = 0.0f;
	float hitReactionRotation_ = 0.0f;
	KamataEngine::Vector3 crystal22StartPosition_{};
	KamataEngine::Vector3 crystal22TargetPosition_{};
	std::vector<size_t> attackOrder_; // クリスタルの落下順序リスト
	size_t currentAttackIndex_ = 0;   // 現在落下開始処理中のインデックス
	float spawnIntervalTimer_ = 0.0f; // クリスタル順次発射間隔タイマー
	float spawnInterval_ = 0.15f;

	// ★ 出現アニメーション用変数
	float spawnTimer_ = 0.0f;
	float spawnDuration_ = 3.0f;                          // 出現にかかる時間（3秒）
	KamataEngine::Vector3 targetScale_{2.0f, 2.0f, 2.0f}; // 目標スケール

	// ★ white1x1.png用テクスチャとスプライト・パーティクル管理
	uint32_t whiteTextureHandle_ = 0;
	KamataEngine::Sprite* spriteHpBG_ = nullptr;         // ★ ラスボスHPバー背景
	KamataEngine::Sprite* spriteHpBar_ = nullptr;        // ★ ラスボスHPバー
	KamataEngine::Sprite* spriteParticle_ = nullptr;     // パーティクル描画用スプライト
	KamataEngine::Sprite* spriteTargetMarker_ = nullptr; // ★ 落下目標予告マーカー描画用スプライト
	std::vector<SpawnParticle> particles_;

	// 各手の初期ローカルオフセット位置
	std::array<KamataEngine::Vector3, kNumHands> handLocalOffsets_;

	// 各クリスタルの初期ローカルオフセット位置
	std::array<KamataEngine::Vector3, kNumCrystals> crystalLocalOffsets_; // ★ クリスタルの初期オフセット (22個)
};
