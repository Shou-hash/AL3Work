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
/// ※ モデルの種類ごとに別のライトグループを持つ（通常 / 奥行きステージ構造 / 奥行き背景(中景) / 奥行き背景(遠景) / ラスボス）。
/// ※ ライティングは「朝（ラスボス出現演出前）」と「夜（ラスボス出現演出後）」を別々に設定し、
///    夜の度合い（nightFactor）で朝から夜へなめらかに切り替わる。
/// ※ 平行光源は 0,1 の 2 本が使用可能。3 本目(index 2)は無効のまま、
///    シェーダーへ渡す設定値（color.x: Lambert / Half Lambert、color.y: 光沢の強さ、color.z: 光沢の鋭さ）の置き場として予約している。
/// </summary>
class LightManager {
public:
	enum class LightingType : uint32_t {
		kLambert = 0,     // 標準ランバート（影がはっきり）
		kHalfLambert = 1, // ハーフランバート（影が柔らかい）
	};

	// ★ ライトグループの種類
	enum class GroupType : uint32_t {
		kWorld = 0,      // 通常（ブロック・プレイヤー・敵・ゴールなど）
		kBackgroundNear, // 奥行きステージ構造（ステージのタイルと同じ大きさのタイル。中景より手前）
		kBackgroundMid,  // 奥行き背景（中景）
		kBackgroundFar,  // 奥行き背景（遠景・一番奥）
		kBoss,           // ラスボス
		kCount,
	};

	// ★ 平行光源 1 つ分の「見た目」（朝用・夜用で別々に持つ）
	struct DirLook {
		KamataEngine::Vector3 direction = {0.0f, -1.0f, 0.0f}; // 光の進む向き
		KamataEngine::Vector3 color = {1.0f, 1.0f, 1.0f};
		float brightness = 1.0f; // 明るさ（色に掛ける倍率）
		float saturation = 1.0f; // 彩度（0.0 = 白黒、1.0 = 元の色）
	};
	struct DirState {
		DirLook morning; // 朝（ラスボス出現演出前）
		DirLook night;   // 夜（ラスボス出現演出後）
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
	// transfer が false の場合は LightGroup への反映（TransferBuffer）を行わない（呼び出し側が後で TransferBuffer を呼ぶときに、同じフレームで 2 回反映しないため）
	void Update(bool transfer = true);

	// 現在の設定と演出状態を LightGroup へ反映する（使われているライトグループだけを更新する）
	void TransferBuffer();

	// モデルにライトグループを設定する（Model::Draw 時に使われる）
	void ApplyToModel(KamataEngine::Model* model, GroupType type = GroupType::kWorld) const;
	const KamataEngine::LightGroup* GetLightGroup(GroupType type) const;

	void SetLightingType(LightingType type) { lightType_ = type; }

	// 光と影の動的演出用
	void EnableSunRotation(bool enable) { isSunRotating_ = enable; }

	// ★ 光の演出の制御
	// 赤いスポットライトの点灯フラグ（false の間はスポットライトを全て消す。朝のライティングは常に有効）
	void SetLightsEnabled(bool enabled) { lightsEnabled_ = enabled; }
	bool IsLightsEnabled() const { return lightsEnabled_; }
	// 夜の度合い（0.0 = 朝、1.0 = 夜）
	void SetNightFactor(float factor) { nightTarget_ = factor; }
	// ラスボスの発狂中はスポットライトを激しく明滅させる
	void SetBossFrenzy(bool frenzy) { isBossFrenzy_ = frenzy; }

	// ★ 覚醒時の終末ライティングフラグ設定（朝・夜から終末へ、なめらかに切り替わる）
	void SetAwakened(bool awakened) { isAwakened_ = awakened; }
	// ★ 覚醒の咆哮の余韻（0.0 ~ 1.0）。ラスボスだけをやわらかく光らせる
	void SetAwakenRoarGlow(float glow) { awakenRoarGlow_ = glow; }
	// ★ 終末の裁きの光の柱が着弾している間の赤いポイントライト（intensity が 0 のときは何もしない）
	void SetDoomPointLight(float intensity, const KamataEngine::Vector3& position);

	// ★ 追従スポットライト（対象の座標を渡すと、対象を斜め上から照らす）
	void SetPlayerSpot(bool active, const KamataEngine::Vector3& targetPos);
	void SetEnemySpot(bool active, const KamataEngine::Vector3& targetPos);
	void SetShieldEnemySpot(bool active, const KamataEngine::Vector3& targetPos);

	// ★ ラスボスの自己発光（スポットライトの代わり。モデル本来の色を保ったまま光らせる）。出現演出が始まったら true
	void SetBossGlow(bool active) { bossGlowActive_ = active; }
	void SetCrystalPointLight(bool active, const KamataEngine::Vector3& position);

	// ★ 流星群攻撃中の finalBossCrystal_22 用ポイントライト（黄色）の設定
	void SetMeteorPointLights(bool active, const std::vector<KamataEngine::Vector3>& positions);

	// ImGui描画関数
	void DrawImGui();

private:
	struct LightSet {
		std::unique_ptr<KamataEngine::LightGroup> group;
		KamataEngine::Vector3 ambientMorning = {0.4f, 0.4f, 0.45f}; // 朝の環境光
		KamataEngine::Vector3 ambientNight = {0.05f, 0.06f, 0.14f}; // 夜の環境光
		std::array<DirState, 2> dirLights;
		std::array<PointState, 3> pointLights;
		std::array<SpotState, 3> spotLights;
	};

	static constexpr int kUsableDirLights = 2; // index 2 は予約

	LightSet& GetSet(GroupType type) { return lightSets_[static_cast<size_t>(type)]; }
	const LightSet& GetSet(GroupType type) const { return lightSets_[static_cast<size_t>(type)]; }

	// 1 つのライトグループへ、演出状態を加味した値を反映する（depthRate: 終末の光の奥行きによる暗さの倍率。奥の背景ほど小さい）
	void ApplyLightSet(LightSet& set, bool isBoss, float depthRate);
	// スポットライトを対象の斜め上に配置して対象に向ける
	void FollowSpot(SpotState& spot, bool active, const KamataEngine::Vector3& targetPos, const KamataEngine::Vector3& offset);
	// ImGui（ライトグループ 1 つ分）
	void DrawLightSetImGui(const char* name, LightSet& set, bool isWorld, int spotCount, const char* const* spotNames);

	std::array<LightSet, static_cast<size_t>(GroupType::kCount)> lightSets_;

	// ★ 軽量化: モデルに設定された（使われている）ライトグループだけを毎フレーム更新する
	mutable std::array<bool, static_cast<size_t>(GroupType::kCount)> setUsed_{};
	bool firstTransferDone_ = false; // 初回の反映（全てのライトグループ）が済んだか

	LightingType lightType_ = LightingType::kHalfLambert;

	// ★ 光の演出用パラメータ
	bool lightsEnabled_ = false; // ゲーム側から指示されるスポットライトの点灯フラグ
	float lightFade_ = 0.0f;     // スポットライトの点灯の進行度（0.0 = 消灯、1.0 = 全点灯）
	float nightTarget_ = 0.0f;   // 夜の度合いの目標値
	float nightCurrent_ = 0.0f;  // 夜の度合いの現在値（なめらかに追従）
	bool isBossFrenzy_ = false;  // ラスボス発狂中フラグ
	bool isAwakened_ = false;    // ★ 追加: 覚醒状態（終末感）フラグ
	float time_ = 0.0f;          // 明滅用タイマー

	// ★ 追加: 終末ライティング（覚醒時）の ImGui 調整用パラメータ
	//   目に優しいように、真っ暗な赤ではなく「くすんだ赤茶の環境光 ＋ 沈みきらない赤い太陽 ＋ 紫がかった補助光」で、赤一色にならないようにする
	KamataEngine::Vector3 awakenedAmbientColor_ = {0.16f, 0.09f, 0.12f}; // ★ 覚醒時の環境光色
	KamataEngine::Vector3 awakenedDirColor0_ = {0.62f, 0.26f, 0.18f};    // ★ 覚醒時のメイン平行光源色（地平線の燃えるような赤い太陽）
	KamataEngine::Vector3 awakenedDirColor1_ = {0.20f, 0.15f, 0.30f};    // ★ 覚醒時のサブ平行光源色（紫がかった補助光で影を起こす）
	KamataEngine::Vector3 awakenedDir0_ = {-0.8f, -0.22f, 0.4f};         // ★ 覚醒時のメイン平行光源の向き
	KamataEngine::Vector3 awakenedDir1_ = {0.5f, -0.3f, 0.8f};           // ★ 覚醒時のサブ平行光源の向き
	KamataEngine::Vector3 awakenedSpotColor_ = {0.65f, 0.30f, 0.24f};    // ★ 覚醒時のスポットライト色
	float awakenedGlowRate_ = 0.75f;                                     // ★ 覚醒時のボス自己発光倍率
	float awakenedPulseSpeed_ = 1.2f;                                    // ★ 覚醒時のパルス明滅速度（ゆっくり呼吸するように）
	float awakenedPulseAmp_ = 0.05f;                                     // ★ 覚醒時のパルス明滅振幅
	float awakenedSmoothing_ = 0.025f;                                   // ★ 終末ライティングへ切り替わる速さ（1 フレームあたりの追従率。小さいほどゆっくり）
	bool awakenedOverride_ = false;                                      // ★ ImGuiからの手動覚醒オーバーライドフラグ
	bool manualAwakened_ = false;                                        // ★ 手動覚醒状態のON/OFF
	float awakenedBlend_ = 0.0f;                                         // ★ 終末ライティングの進行度（0.0 = 朝・夜、1.0 = 終末。なめらかに追従）
	float awakenRoarGlow_ = 0.0f;                                        // ★ 覚醒の咆哮の余韻（ラスボスの発光に足す）

	// ★ ラスボスの自己発光（シェーダーの pointLights[2].intensity 経由で渡す）
	bool bossGlowActive_ = false;   // 発光させるかどうか
	float bossGlowFade_ = 0.0f;     // 発光の進行度（0.0 ~ 1.0。なめらかに点灯する）
	float bossGlowStrength_ = 2.8f; // 発光の強さ（ImGui の Boss Glow Strength で調整）

	// ★ 光沢（全モデル共通）。シェーダーの鏡面反射に使われる
	float glossStrength_ = 0.25f;  // 光沢の強さ（0.0 = 光沢なし。ImGui の Gloss Strength で調整）
	float glossShininess_ = 32.0f; // 光沢の鋭さ（大きいほどハイライトが小さく鋭い。ImGui の Gloss Shininess で調整）

	// ★ ラスボス専用のハイライト（光沢・鏡面反射）パラメータ
	float bossGlossStrength_ = 1.2f;   // ラスボスのハイライトの強さ（くっきりと光らせる）
	float bossGlossShininess_ = 16.0f; // ラスボスのハイライトの広がり・鋭さ

	// 追従スポットライトの配置オフセット（対象からの相対位置）
	KamataEngine::Vector3 spotOffset_ = {0.0f, 5.0f, -8.0f};

	// ImGui からの手動操作（ゲーム側の指示より優先）
	bool manualOverride_ = false;
	bool manualEnabled_ = true;
	float manualNight_ = 0.0f;

	// 太陽の回転演出（朝の太陽が、左（日の出）➔ 上（昼）➔ 右（日の入り）➔ 下（地平線の下）と円を描いて一周する）
	bool isSunRotating_ = true; // ONだと毎フレーム太陽の向きが上書きされる（ImGui で向きを手動調整すると OFF になる）
	float sunAngle_ = 0.3f;     // 太陽の角度（0 = 左の地平線、π/2 = 真上、π = 右の地平線、3π/2 = 真下）

	// ★ 太陽の上昇・下降による光の変化（朝の太陽の回転中に毎フレーム更新される）
	float sunBrightnessRate_ = 1.0f; // 太陽光の明るさの倍率（地平線の下では暗くなる）
	float sunWarmRate_ = 0.0f;       // 朝焼け・夕焼けの色味の強さ（太陽が低いほど強い）
	float sunAmbientRate_ = 1.0f;    // 環境光の明るさの倍率（昼は明るく、地平線の下では少し暗くなる）
};
