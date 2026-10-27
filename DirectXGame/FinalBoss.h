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
		kStandby,       // 待機状態（プレイヤー接近前・完全非表示）
		kSpawn,         // 出現アニメーション中
		kNormal,        // 通常状態
		kFrenzyInit,    // 発狂初期演出中（振動・溜め）
		kFrenzy,        // 発狂待機状態（攻撃インターバル待ち）
		kAwakening,     // ★ 覚醒アニメーション演出中（カメラ演出・終末ライティング切替）
		kDodgingAttack, // ★ 覚醒専用回避攻撃（避けるしかできない広範囲攻撃）
		kAttack,        // ★ 流星群（クリスタル落下）攻撃状態
		kDownWarning,   // ★ ダウン予告状態（苦しんでフラフラ・大揺れしてダウンを予兆）
		kDown,          // ★ ダウン状態（衝撃後に伏せて本体攻撃受付中）
		kDefeat         // ★ 撃破アニメーション中（痙攣 ➔ 崩れ落ちる ➔ 縮小して消える）
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
		KamataEngine::Vector3 targetPosition{}; // ★ 着地予定座標（予告マーカー用）
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

	// ★ 覚醒専用攻撃「終末の裁き」の柱（クリスタル照射）の状態定義
	enum class DoomPillarStatus {
		kInactive,  // 未使用
		kFalling,   // ★ 空から落下して空中静止
		kTelegraph, // ★ 空中静止＆下の先端から画像の射線照射
		kStrike,    // ★ 着弾中（当たり判定あり）
		kReturning  // ★ 射線が消えて上空へ復帰
	};

	// ★ 覚醒専用攻撃「終末の裁き」の柱 1 本分のデータ
	struct DoomPillar {
		DoomPillarStatus status = DoomPillarStatus::kInactive;
		float x = 0.0f;                 // 柱の中心X座標
		float groundY = 0.0f;           // 柱が着弾する地面の高さ
		float targetY = 0.0f;           // ★ 空中静止する目標Y座標（一定値の中でランダム）
		float z = 0.0f;                 // ★ 手前描画用のZ座標
		float timer = 0.0f;             // 各状態の経過時間
		float fallDuration = 0.35f;     // ★ 個別の落下時間（進行時間に伴い高速化）
		float telegraphDuration = 0.8f; // ★ 個別の射線照射予告時間（進行時間に伴い高速化）
		bool hitDone = false;           // この柱でプレイヤーに当たったか（多重ヒット防止）
		// ★ 空中静止＆照射用 finalBossCrystal_22 のトランスフォームと座標
		KamataEngine::WorldTransform crystalTransform{};
		float crystalY = 0.0f; // クリスタルの現在のY座標
	};

	FinalBoss() = default;
	~FinalBoss() override; // ★ スプライト解放用デストラクタ

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(
	    KamataEngine::Model* modelBody, const std::array<KamataEngine::Model*, kNumHands>& modelHands, const std::array<KamataEngine::Model*, kNumCrystals>& modelCrystals,
	    KamataEngine::Model* modelCrystal22, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

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

	// ★ 覚醒状態のトリガー＆判定アクセサ
	void TriggerAwakening();                                                       // ★ 覚醒状態へ移行
	bool IsAwakened() const { return isAwakened_; }                                // ★ 覚醒済みか
	bool IsAwakening() const { return state_ == State::kAwakening; }               // ★ 覚醒演出中か
	bool IsDodgingAttackActive() const { return state_ == State::kDodgingAttack; } // ★ 回避専用攻撃中か

	// ★ 覚醒演出の進行度（0.0 ~ 1.0）。カメラ・ライティングの演出用
	float GetAwakeningProgress() const { return state_ == State::kAwakening ? (std::min)(awakeningTimer_ / kAwakeningDuration, 1.0f) : (isAwakened_ ? 1.0f : 0.0f); }
	// ★ 覚醒の咆哮の余韻（0.0 ~ 1.0。咆哮の瞬間が 1.0 で、なめらかに 0.0 へ戻る）。ライティングの演出用
	float GetAwakenRoarGlow() const { return awakenRoarGlow_; }

	// ★ 終末の裁き（覚醒専用攻撃）の演出用アクセサ
	// 着弾の強さ（0.0 ~ 1.0）。ライティングの演出用
	float GetDoomStrikeIntensity() const { return doomStrikeIntensity_; }
	// 着弾した瞬間に true を返す（1 回だけ）。outPos にプレイヤーに一番近い柱の位置を返す。カメラ演出用
	bool ConsumeDoomStrikeEvent(KamataEngine::Vector3& outPos) {
		if (!doomStrikeEvent_) {
			return false;
		}
		doomStrikeEvent_ = false;
		outPos = doomStrikeEventPos_;
		return true;
	}
	// 着弾中の柱の光源位置（プレイヤーに一番近い柱）を取得する。着弾中の柱が無ければ false
	bool GetDoomLightPosition(KamataEngine::Vector3& outPos) const {
		outPos = doomLightPos_;
		return doomStrikeIntensity_ > 0.01f;
	}

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

	// ★ 撃破後の処刑演出用
	bool IsExecuting() const { return isExecuting_; }
	void BeginExecution();
	void SetExecutionCharge(float charge) { executionCharge_ = (std::clamp)(charge, 0.0f, 1.0f); }
	void ExecutionStrike(float directionX);
	void EndExecution();

	// ★ 本体への多重ヒット防止フラグのリセット・取得
	void ResetBodyHitConsumed() { bodyHitConsumed_ = false; }
	bool IsBodyHitConsumed() const { return bodyHitConsumed_; }

	// ★ 撃破アニメーションの進行度（0.0 ~ 1.0）と終了判定
	float GetDefeatProgress() const { return state_ == State::kDefeat ? (std::min)(defeatTimer_ / kDefeatDuration, 1.0f) : (isDefeatAnimationFinished_ ? 1.0f : 0.0f); }
	bool IsDefeatAnimationFinished() const { return isDefeatAnimationFinished_; }

	// ★ カメラのズームアウト完了フラグのセット・取得
	void SetDefeatZoomFinished(bool finished) { isDefeatZoomFinished_ = finished; }
	bool IsDefeatZoomFinished() const { return isDefeatZoomFinished_; }

	// ★ 流星群が降り始めたか
	bool IsMeteorShowerStarted() const { return meteorShowerStarted_; }
	int GetFinalBossHp() const { return finalBossHp_; }
	int GetMaxFinalBossHp() const { return maxFinalBossHp_; }
	bool ConsumeFinalBossHit() {
		bool hit = finalBossHitEvent_;
		finalBossHitEvent_ = false;
		return hit;
	}

	// ★ 出現演出の進行度（0.0 ~ 1.0）を返す
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
	void EmitDefeatParticles();
	void UpdateParticles();
	void DrawParticles();

	// ★ 落下目標予告マーカーの描画処理
	void DrawTargetMarkers();

	// ★ 3Dワールド座標から2Dスクリーン座標への変換ヘルパー関数
	KamataEngine::Vector2 WorldToScreen(const KamataEngine::Vector3& worldPos, const KamataEngine::Camera& camera);

	// 描画座標調整用オフセットのアクセサ
	KamataEngine::Vector3& GetDrawOffset() { return drawOffset_; }

	// ImGui調整用アクセサ
	KamataEngine::Vector3& GetBasePosition() { return basePosition_; }
	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	KamataEngine::Matrix4x4 MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b);

	// ★ 覚醒専用攻撃「終末の裁き」の処理
	void StartDodgingAttack();                                  // ★ 攻撃の開始（溜めから）
	void UpdateDodgingAttack(float dt);                         // ★ 攻撃の更新（クリスタルの単発落下・照射・復帰・終了）
	void SpawnSingleDoomPillar();                               // ★ 単発のクリスタルを降下・設置する
	float FindGroundY(float x, float referenceY) const;         // ★ 指定したX座標の、基準の高さ以下で一番高い地面の高さを探す
	void EmitDoomSparks(float x, float groundY);                // ★ 着弾時の火の粉
	void ApplyAwakenBodyPose();                                 // ★ 覚醒演出・終末の裁きの本体の姿勢（位置・傾き・大きさ）
	KamataEngine::Vector3 GetAnchoredPosition() const;          // ★ カメラの視差に合わせた定位置（覚醒開始時のずれの計算用）
	void DrawDoomPillars(const KamataEngine::Matrix4x4& matVP); // ★ 光の柱（画像の射線）の描画（スプライト）

	KamataEngine::Camera* camera_ = nullptr;
	Player* player_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	// モデル参照
	KamataEngine::Model* modelBody_ = nullptr;
	std::array<KamataEngine::Model*, kNumHands> modelHands_{};
	std::array<KamataEngine::Model*, kNumCrystals> modelCrystals_{};
	KamataEngine::Model* modelCrystal22_ = nullptr;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformBody_;
	std::array<KamataEngine::WorldTransform, kNumHands> worldTransformHands_;
	std::array<KamataEngine::WorldTransform, kNumCrystals> worldTransformCrystals_;
	KamataEngine::WorldTransform worldTransformCrystal22_;

	KamataEngine::WorldTransform particleWorldTransform_;

	KamataEngine::Vector3 basePosition_{};
	KamataEngine::Vector3 drawOffset_{-33.0f, -7.8f, 0.0f};
	float parallaxFactor_ = 0.35f;

	float animTimer_ = 0.0f;
	State state_ = State::kStandby;
	float frenzyTimer_ = 0.0f;

	// ★ 覚醒状態用変数
	bool isAwakened_ = false;                         // ★ 覚醒しているかフラグ
	float awakeningTimer_ = 0.0f;                     // ★ 覚醒アニメーションタイマー
	static constexpr float kAwakeningDuration = 4.2f; // ★ 覚醒演出時間（4.2秒：うなだれ ➔ 胎動 ➔ 咆哮 ➔ 余韻）
	bool awakenRoared_ = false;                       // ★ 覚醒の咆哮を済ませたか
	float awakenRoarGlow_ = 0.0f;                     // ★ 咆哮の余韻（ライティング用）
	float awakenStartRotX_ = 0.0f;                    // ★ 覚醒開始時の本体の傾き（伏せ状態からなめらかに起き上がるため）
	KamataEngine::Vector3 awakenRiseOffset_{};        // ★ 覚醒開始時の位置のずれ（伏せていた位置からなめらかに定位置へ戻すため）

	// ★ 覚醒専用攻撃「終末の裁き」用変数（クリスタルが1個ずつ落下・照射・復帰を繰り返す）
	float dodgingAttackTimer_ = 0.0f;                       // ★ 回避専用攻撃タイマー
	static constexpr size_t kMaxDoomPillars = 10;           // ★ 同時に存在できる柱の最大数
	static constexpr int kDoomTotalCount = 6;               // ★ クリスタルが順番に落ちて照射する合計回数
	static constexpr float kDoomChargeDuration = 0.8f;      // ★ 溜め（最初の予備動作）の時間
	static constexpr float kDoomFallDuration = 0.35f;       // ★ 空から落下して静止するまでの基本時間
	static constexpr float kDoomTelegraphDuration = 0.8f;   // ★ 空中静止して射線を照射する基本時間
	static constexpr float kDoomStrikeDuration = 0.35f;     // ★ 着弾・当たり判定時間
	static constexpr float kDoomReturnDuration = 0.4f;      // ★ 射線が消えて上に戻る時間
	static constexpr float kDoomPillarHalfWidth = 0.6f;     // ★ 柱自体の判定基準半径（適正サイズに補正）
	int doomCount_ = 0;                                     // ★ 現在何個目のクリスタル攻撃か
	float doomPunch_ = 0.0f;                                // ★ 着弾の瞬間に本体が反動で膨らむ演出（0.0 ~ 1.0）
	float doomStrikeIntensity_ = 0.0f;                      // ★ 着弾の強さ（ライティング用。なめらかに追従）
	bool doomStrikeEvent_ = false;                          // ★ 着弾した瞬間のフラグ（カメラ用）
	KamataEngine::Vector3 doomStrikeEventPos_{};            // ★ 着弾した柱の位置（カメラ用）
	KamataEngine::Vector3 doomLightPos_{};                  // ★ 着弾中の柱の光源位置（ライティング用）
	std::array<DoomPillar, kMaxDoomPillars> doomPillars_{}; // ★ 柱のプール

	// ★ 流星群攻撃管理用変数
	float frenzyWaitTimer_ = 0.0f;
	float frenzyWaitDuration_ = 6.0f;

	bool attackDamageDone_ = false;

	// ★ ダウン予告演出用変数
	float downWarningTimer_ = 0.0f;
	static constexpr float kDownWarningDuration = 1.2f;

	// ★ ダウン（伏せる）演出用変数
	bool pendingDown_ = false;
	float downTimer_ = 0.0f;
	float downDuration_ = 3.5f;
	bool bodyHitConsumed_ = false;
	KamataEngine::Vector3 downStartPosition_{};
	KamataEngine::Vector3 downTargetPosition_{};

	// ★ 攻撃開始・攻撃中の本体アニメーション用
	float attackTimer_ = 0.0f;
	static constexpr float kAttackIntroDuration = 1.2f;

	// ★ 撃破アニメーション用
	float defeatTimer_ = 0.0f;
	bool isDefeatAnimationFinished_ = false;
	bool isDefeatZoomFinished_ = false;
	static constexpr float kDefeatDuration = 4.0f;
	bool meteorShowerStarted_ = false;

	// ★ 処刑演出用
	bool isExecuting_ = false;
	bool executionStruck_ = false;
	float executionCharge_ = 0.0f;
	float executionSpin_ = 0.0f;
	float executionStrikeTimer_ = 0.0f;
	float executionStrikeDirX_ = 1.0f;
	static constexpr float kExecutionShatterDuration = 0.7f;

	std::array<CrystalAttackInfo, kNumCrystals> crystalAttackInfos_{};
	std::array<MeteorCrystal, kNumMeteorShower> meteorCrystals_{};
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
	std::vector<size_t> attackOrder_;
	size_t currentAttackIndex_ = 0;
	float spawnIntervalTimer_ = 0.0f;
	float spawnInterval_ = 0.15f;

	// ★ 出現アニメーション用変数
	float spawnTimer_ = 0.0f;
	float spawnDuration_ = 3.0f;
	KamataEngine::Vector3 targetScale_{2.0f, 2.0f, 2.0f};

	// ★ テクスチャとスプライト・パーティクル管理
	uint32_t whiteTextureHandle_ = 0;
	KamataEngine::Sprite* spriteHpBG_ = nullptr;
	KamataEngine::Sprite* spriteHpBar_ = nullptr;
	KamataEngine::Sprite* spriteParticle_ = nullptr;
	KamataEngine::Sprite* spriteTargetMarker_ = nullptr;
	std::vector<SpawnParticle> particles_;

	std::array<KamataEngine::Vector3, kNumHands> handLocalOffsets_;
	std::array<KamataEngine::Vector3, kNumCrystals> crystalLocalOffsets_;
};