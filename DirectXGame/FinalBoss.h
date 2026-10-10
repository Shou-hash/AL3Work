#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <vector>

class MapChipField;
class Player;

class FinalBoss : public BaseEnemy {
public:
	static constexpr size_t kNumHands = 7;

	// ★ ボスの状態定義
	enum class State {
		kStandby,    // 待機状態（プレイヤー接近前・完全非表示）
		kSpawn,      // 出現アニメーション中
		kNormal,     // 通常状態
		kFrenzyInit, // 発狂初期演出中（振動・溜め）
		kFrenzy      // 発狂攻撃状態
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
	/// <param name="camera">カメラ</param>
	/// <param name="position">初期配置座標</param>
	void Initialize(KamataEngine::Model* modelBody, const std::array<KamataEngine::Model*, kNumHands>& modelHands, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update() override;
	void Draw() override;
	void OnCollision(Player* player) override;
	void OnDead() override;
	AABB GetAABB() const override;

	void SetPlayer(Player* player) { player_ = player; }

	// 出現・発狂アニメーション制御関数
	void StartSpawn();  // ★ 出現アニメーション開始
	void StartFrenzy(); // 発狂アニメーション開始
	State GetState() const { return state_; }

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
	void UpdateParticles();
	void DrawParticles();

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

	// モデル参照
	KamataEngine::Model* modelBody_ = nullptr;
	std::array<KamataEngine::Model*, kNumHands> modelHands_{};

	// ワールドトランスフォーム
	// worldTransform_ (親 / ルート座標)
	KamataEngine::WorldTransform worldTransformBody_;                         // 本体ローカル
	std::array<KamataEngine::WorldTransform, kNumHands> worldTransformHands_; // 手のローカル＆合成用

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

	// ★ 出現アニメーション用変数
	float spawnTimer_ = 0.0f;
	float spawnDuration_ = 3.0f;                          // 出現にかかる時間（3秒）
	KamataEngine::Vector3 targetScale_{2.0f, 2.0f, 2.0f}; // 目標スケール

	// ★ white1x1.png用テクスチャとスプライト・パーティクル管理
	uint32_t whiteTextureHandle_ = 0;
	KamataEngine::Sprite* spriteParticle_ = nullptr; // ★ パーティクル描画用スプライト
	std::vector<SpawnParticle> particles_;

	// 各手の初期ローカルオフセット位置
	std::array<KamataEngine::Vector3, kNumHands> handLocalOffsets_;
};