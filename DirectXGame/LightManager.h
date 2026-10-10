#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstdint>
#include <memory>

/// <summary>
/// ライト管理クラス
/// エンジン標準の KamataEngine::LightGroup を使い、ImGui の調整値と光の演出を毎フレーム反映する。
/// ※ Model::Draw は内部で model->lightGroup_ をバインドするため、
///    各モデルに ApplyToModel() で LightGroup を設定する必要がある。
/// ※ モデルの種類ごとに別のライトグループを持つ（通常 / 奥行き背景(中景) / 奥行き背景(遠景) / ラスボス）。
/// ※ 平行光源は 0,1 の 2 本が使用可能。3 本目(index 2)は
///    Lambert / Half Lambert の切替フラグ置き場として予約している(シェーダー側で読み取る)。
/// </summary>
class LightManager {
public:
	enum class LightingType : uint32_t {
		kLambert = 0,     // 標準ランバート（影がはっきり）
		kHalfLambert = 1, // ハーフランバート（影が柔らかい）
	};

	// ★ ライトグループの種類
	enum class GroupType : uint32_t {
		kWorld = 0,     // 通常（ブロック・プレイヤー・敵・ゴールなど）
		kBackgroundMid, // 奥行き背景（中景）
		kBackgroundFar, // 奥行き背景（遠景・一番奥）
		kBoss,          // ラスボス
		kCount,
	};

	struct DirState {
		KamataEngine::Vector3 direction = {0.0f, -1.0f, 0.0f};
		KamataEngine::Vector3 color = {1.0f, 1.0f, 1.0f};
		float brightness = 1.0f; // 明るさ（色に掛ける倍率）
		float saturation = 1.0f; // 彩度（0.0 = 白黒、1.0 = 元の色）
		bool active = false;
	};
	struct PointState {
		KamataEngine::Vector3 color = {1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;
		KamataEngine::Vector3 position = {0.0f, 0.0f, 0.0f};
		float radius = 10.0f;
		float decay = 1.0f;
		bool active = false;
	};
	struct SpotState {
		KamataEngine::Vector3 color = {1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;
		KamataEngine::Vector3 position = {0.0f, 0.0f, 0.0f};
		float radius = 10.0f;
		KamataEngine::Vector3 direction = {0.0f, -1.0f, 0.0f};
		float decay = 1.0f;
		KamataEngine::Vector2 angleDeg = {25.0f, 60.0f}; // x: 減衰開始角度(内側) y: 減衰終了角度(外側)  単位は度
		bool active = false;
	};

	void Initialize();
	void Update();

	// 現在の設定と演出状態を LightGroup へ反映する
	void TransferBuffer();

	// モデルにライトグループを設定する（Model::Draw 時に使われる）
	void ApplyToModel(KamataEngine::Model* model, GroupType type = GroupType::kWorld) const;
	const KamataEngine::LightGroup* GetLightGroup(GroupType type) const;

	void SetLightingType(LightingType type) { lightType_ = type; }

	// 光と影の動的演出用
	void EnableSunRotation(bool enable) { isSunRotating_ = enable; }

	// ★ 光の演出の制御
	// false の間は全てのライトを無効にし、一様な環境光のみで描画する（光の描画なし）
	void SetLightsEnabled(bool enabled) { lightsEnabled_ = enabled; }
	bool IsLightsEnabled() const { return lightsEnabled_; }
	// 夜の度合い（0.0 = 昼、1.0 = 夜）
	void SetNightFactor(float factor) { nightTarget_ = factor; }
	// ラスボスの発狂中はスポットライトを激しく明滅させる
	void SetBossFrenzy(bool frenzy) { isBossFrenzy_ = frenzy; }

	// ★ 追従スポットライト（対象の座標を渡すと、対象を斜め上から照らす）
	void SetPlayerSpot(bool active, const KamataEngine::Vector3& targetPos);
	void SetEnemySpot(bool active, const KamataEngine::Vector3& targetPos);
	void SetShieldEnemySpot(bool active, const KamataEngine::Vector3& targetPos);
	void SetBossSpot(bool active, const KamataEngine::Vector3& targetPos);

	// ImGui描画関数
	void DrawImGui();

private:
	struct LightSet {
		std::unique_ptr<KamataEngine::LightGroup> group;
		KamataEngine::Vector3 ambientColor = {0.3f, 0.3f, 0.3f};
		std::array<DirState, 2> dirLights;
		std::array<PointState, 3> pointLights;
		std::array<SpotState, 3> spotLights;
	};

	static constexpr int kUsableDirLights = 2; // index 2 は予約

	LightSet& GetSet(GroupType type) { return lightSets_[static_cast<size_t>(type)]; }
	const LightSet& GetSet(GroupType type) const { return lightSets_[static_cast<size_t>(type)]; }

	// 1 つのライトグループへ、演出状態を加味した値を反映する
	void ApplyLightSet(LightSet& set, bool isBoss);
	// スポットライトを対象の斜め上に配置して対象に向ける
	void FollowSpot(SpotState& spot, bool active, const KamataEngine::Vector3& targetPos, const KamataEngine::Vector3& offset);
	// ImGui（ライトグループ 1 つ分）
	void DrawLightSetImGui(const char* name, LightSet& set, bool isWorld, int spotCount, const char* const* spotNames);

	std::array<LightSet, static_cast<size_t>(GroupType::kCount)> lightSets_;

	LightingType lightType_ = LightingType::kHalfLambert;

	// ★ 光の演出用パラメータ
	bool lightsEnabled_ = false; // ゲーム側から指示される点灯フラグ
	float lightFade_ = 0.0f;     // 点灯の進行度（0.0 = 消灯、1.0 = 全点灯）
	float nightTarget_ = 0.0f;   // 夜の度合いの目標値
	float nightCurrent_ = 0.0f;  // 夜の度合いの現在値（なめらかに追従）
	bool isBossFrenzy_ = false;  // ラスボス発狂中フラグ
	float time_ = 0.0f;          // 明滅用タイマー

	// 光が消えている間の一様な環境光
	KamataEngine::Vector3 unlitAmbient_ = {1.0f, 1.0f, 1.0f};
	// 夜になったときの環境光・平行光源に掛ける色（青暗くなる）
	KamataEngine::Vector3 nightAmbientMul_ = {0.15f, 0.18f, 0.40f};
	KamataEngine::Vector3 nightDirMul_ = {0.12f, 0.16f, 0.38f};

	// 追従スポットライトの配置オフセット（対象からの相対位置）
	KamataEngine::Vector3 spotOffset_ = {0.0f, 5.0f, -8.0f};
	KamataEngine::Vector3 bossSpotOffset_ = {0.0f, 6.0f, -12.0f};

	// ImGui からの手動操作（ゲーム側の指示より優先）
	bool manualOverride_ = false;
	bool manualEnabled_ = true;
	float manualNight_ = 0.0f;

	// 太陽の回転演出
	bool isSunRotating_ = false; // ONだと毎フレーム方向が上書きされるのでデフォルトOFF
	float sunAngle_ = 0.0f;
};
