#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <array>

class MapChipField;
class Player;

class FinalBoss : public BaseEnemy {
public:
	static constexpr size_t kNumHands = 7;

	// ★ 追加: ボスの状態定義
	enum class State {
		kNormal,     // 通常状態
		kFrenzyInit, // 発狂初期演出中（振動・溜め）
		kFrenzy      // 発狂攻撃状態
	};

	FinalBoss() = default;
	~FinalBoss() override = default;

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

	// 発狂アニメーション開始関数
	void StartFrenzy();
	State GetState() const { return state_; }

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

	// 奥行き・背景（パララックス）用変数 (BackgroundBlocksを参考)
	KamataEngine::Vector3 basePosition_{};

	// 見た目（描画位置）のみをずらすための調整用オフセット
	KamataEngine::Vector3 drawOffset_{-33.0f, -7.8f, 0.0f};

	float parallaxFactor_ = 0.35f; // 視差係数（手前よりゆっくり動き、奥深さを演出）

	// アニメーションタイマー
	float animTimer_ = 0.0f;
	State state_ = State::kNormal; // ★ 追加
	float frenzyTimer_ = 0.0f;     // ★ 追加: 発狂初期演出用タイマー

	// 各手の初期ローカルオフセット位置
	std::array<KamataEngine::Vector3, kNumHands> handLocalOffsets_;
};