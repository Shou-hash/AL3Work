#include "LightManager.h"
#include <cmath>
#include <numbers>
#include <string>

using namespace KamataEngine;

namespace {

// SetSpotLightFactorAngle に渡す角度の単位。
// 円錐の広がりがおかしい場合(極端に狭い/広い)は false に変更してください(ラジアン扱い)。
constexpr bool kSpotAngleInDegrees = true;

// ライト方向の符号。ライトが逆向きに当たる場合は -1.0f に変更してください。
// 平行光源：1.0f = 「光の進む向き」を渡す想定 / スポットライト：1.0f = 「光の進む向き」を渡す想定
constexpr float kDirLightSign = 1.0f;
constexpr float kSpotLightSign = 1.0f;

// スポットライトが全点灯するまでの時間（秒）
constexpr float kFadeDuration = 1.5f;
// 夜の度合いが目標値に追従する速さ
constexpr float kNightSmoothing = 0.08f;

// スポットライトの赤い色（少し赤い）
const Vector3 kSpotRedColor = {1.0f, 0.55f, 0.5f};

float ToEngineAngle(float deg) { return kSpotAngleInDegrees ? deg : deg * (std::numbers::pi_v<float> / 180.0f); }

// 正規化（ゼロ長ベクトル対策つき）
Vector3 SafeDir(const Vector3& v, const Vector3& fallback, float sign) {
	float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len < 0.0001f) {
		return fallback;
	}
	return {v.x / len * sign, v.y / len * sign, v.z / len * sign};
}

Vector3 Lerp(const Vector3& a, const Vector3& b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t}; }

Vector3 Scale(const Vector3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }

// スムーズステップ（edge0 以下で 0.0、edge1 以上で 1.0 になめらかに変化する）
float SmoothStep(float edge0, float edge1, float x) {
	float t = (x - edge0) / (edge1 - edge0);
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	return t * t * (3.0f - 2.0f * t);
}

// ★ 太陽の回転演出のパラメータ
constexpr float kSunPeriodSeconds = 60.0f;         // 太陽が円を一周するのにかかる時間（秒）。大きいほどゆっくり回る
constexpr float kSunDepth = 0.35f;                 // 光の奥行き成分（正面の面にも少し光を当てて、のっぺりしないようにする）
const Vector3 kSunWarmColor = {1.0f, 0.5f, 0.25f}; // 朝焼け・夕焼けの色

// 彩度と明るさを調整した色を返す
Vector3 AdjustColor(const Vector3& color, float brightness, float saturation) {
	float gray = color.x * 0.299f + color.y * 0.587f + color.z * 0.114f;
	Vector3 gray3 = {gray, gray, gray};
	return Scale(Lerp(gray3, color, saturation), brightness);
}

void InitDirLook(LightManager::DirLook& look, const Vector3& direction, const Vector3& color, float brightness, float saturation) {
	look.direction = direction;
	look.color = color;
	look.brightness = brightness;
	look.saturation = saturation;
}

// 少し赤いスポットライトの初期設定
void InitRedSpot(LightManager::SpotState& s, float intensity, float radius) {
	s.color = kSpotRedColor;
	s.intensity = intensity;
	s.radius = radius;
	s.decay = 1.0f;
	s.angleDeg = {15.0f, 40.0f};
	s.active = false;
}

} // namespace

void LightManager::SetCrystalPointLight(bool active, const KamataEngine::Vector3& position) {
	for (GroupType type : {GroupType::kWorld, GroupType::kBoss}) {
		PointState& p = GetSet(type).pointLights[0];
		p.active = active;
		p.color = {0.25f, 0.85f, 1.0f};
		p.intensity = active ? 8.0f : 0.0f;
		p.position = position;
		p.radius = 18.0f;
		p.decay = 1.5f;
	}
}

void LightManager::Initialize() {
	// エンジン標準のライトグループを生成（種類ごとに 1 つ）
	for (auto& set : lightSets_) {
		set.group.reset(LightGroup::Create());
	}

	lightType_ = LightingType::kHalfLambert;
	isSunRotating_ = true; // ★ 朝の太陽をゆっくり回転させる
	lightsEnabled_ = false;
	lightFade_ = 0.0f;
	nightTarget_ = 0.0f;
	nightCurrent_ = 0.0f;
	isBossFrenzy_ = false;
	time_ = 0.0f;

	// =========================================================
	// 通常（ブロック・プレイヤー・敵など）
	// 朝：低い位置の金色の太陽を横から当てて明暗をはっきり出し、手前からは紫がかった柔らかい空の光で影を起こす
	// 夜：青白い月明かり
	// ※ 朝の光を画面の手前（カメラ側）から当てると、のっぺりして朝らしさが見えなくなるため、横から当てる
	// =========================================================
	LightSet& worldLights = GetSet(GroupType::kWorld);
	worldLights.ambientMorning = {0.48f, 0.42f, 0.48f};
	worldLights.ambientNight = {0.05f, 0.06f, 0.14f};
	// メインの平行光源（朝の太陽：右の低い位置から、ほぼ真横に差し込む）
	worldLights.dirLights[0].active = true;
	InitDirLook(worldLights.dirLights[0].morning, {-0.9f, -0.3f, 0.05f}, {1.0f, 0.68f, 0.38f}, 1.5f, 1.0f);
	InitDirLook(worldLights.dirLights[0].night, {0.4f, -0.7f, 0.6f}, {0.5f, 0.6f, 1.0f}, 0.3f, 0.8f);
	// 補助光（朝の空の紫がかった反射光。手前から当てて影の側を柔らかく起こす）
	worldLights.dirLights[1].active = true;
	InitDirLook(worldLights.dirLights[1].morning, {0.5f, -0.2f, 0.8f}, {0.65f, 0.6f, 1.0f}, 0.55f, 0.7f);
	InitDirLook(worldLights.dirLights[1].night, {0.6f, -0.3f, 0.7f}, {0.2f, 0.25f, 0.5f}, 0.1f, 0.6f);
	InitRedSpot(worldLights.spotLights[0], 2.0f, 25.0f); // プレイヤー
	InitRedSpot(worldLights.spotLights[1], 2.0f, 25.0f); // 普通敵
	InitRedSpot(worldLights.spotLights[2], 2.0f, 25.0f); // 盾敵

	// =========================================================
	// 奥行きステージ構造（ステージのタイルと同じ大きさ）：通常より少し落ち着いた明るさ・彩度
	// =========================================================
	LightSet& nearLights = GetSet(GroupType::kBackgroundNear);
	nearLights.ambientMorning = {0.46f, 0.42f, 0.46f};
	nearLights.ambientNight = {0.045f, 0.055f, 0.13f};
	nearLights.dirLights[0].active = true;
	InitDirLook(nearLights.dirLights[0].morning, {-0.9f, -0.3f, 0.1f}, {1.0f, 0.7f, 0.42f}, 1.3f, 0.9f);
	InitDirLook(nearLights.dirLights[0].night, {0.4f, -0.7f, 0.6f}, {0.5f, 0.6f, 1.0f}, 0.25f, 0.8f);

	// =========================================================
	// 奥行き背景（中景）：やや明るく、彩度はそこそこ
	// =========================================================
	LightSet& midLights = GetSet(GroupType::kBackgroundMid);
	midLights.ambientMorning = {0.44f, 0.42f, 0.46f};
	midLights.ambientNight = {0.04f, 0.05f, 0.12f};
	midLights.dirLights[0].active = true;
	InitDirLook(midLights.dirLights[0].morning, {-0.9f, -0.25f, 0.1f}, {1.0f, 0.72f, 0.45f}, 1.1f, 0.75f);
	InitDirLook(midLights.dirLights[0].night, {0.3f, -0.6f, 0.7f}, {0.45f, 0.55f, 0.95f}, 0.2f, 0.6f);

	// =========================================================
	// 奥行き背景（遠景・一番奥）：一番暗く、彩度も一番低い（朝の霞）
	// =========================================================
	LightSet& farLights = GetSet(GroupType::kBackgroundFar);
	farLights.ambientMorning = {0.36f, 0.35f, 0.40f};
	farLights.ambientNight = {0.02f, 0.03f, 0.07f};
	farLights.dirLights[0].active = true;
	InitDirLook(farLights.dirLights[0].morning, {-0.9f, -0.25f, 0.1f}, {1.0f, 0.85f, 0.75f}, 0.65f, 0.3f);
	InitDirLook(farLights.dirLights[0].night, {0.3f, -0.6f, 0.7f}, {0.5f, 0.55f, 0.8f}, 0.08f, 0.2f);

	// =========================================================
	// ラスボス：朝は通常に近く、夜は不気味な赤紫の光
	// =========================================================
	LightSet& bossLights = GetSet(GroupType::kBoss);
	bossLights.ambientMorning = {0.55f, 0.50f, 0.58f}; // ★ 環境光（朝）
	bossLights.ambientNight = {0.20f, 0.10f, 0.20f};   // ★ 環境光（夜）

	// メイン平行光源
	bossLights.dirLights[0].active = true;
	InitDirLook(bossLights.dirLights[0].morning, {-0.9f, -0.3f, 0.05f}, {1.0f, 1.0f, 1.0f}, 1.6f, 1.0f); // ★ 光の色を白(1.0, 1.0, 1.0)に変更
	InitDirLook(bossLights.dirLights[0].night, {0.0f, -0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}, 1.33f, 1.0f);    // ★ 光の色を白(1.0, 1.0, 1.0)に変更

	// ★ ラスボスのハイライト用補助光源（手前・斜め上から照らして鮮やかな光沢を出す）
	bossLights.dirLights[1].active = true;
	InitDirLook(bossLights.dirLights[1].morning, {0.4f, -0.6f, -0.8f}, {1.0f, 1.0f, 1.0f}, 1.2f, 1.0f); // ★ 光の色を白(1.0, 1.0, 1.0)に変更
	InitDirLook(bossLights.dirLights[1].night, {0.3f, -0.5f, -0.8f}, {1.0f, 1.0f, 1.0f}, 0.063f, 1.0f); // ★ 光の色を白(1.0, 1.0, 1.0)に変更

	TransferBuffer();
}

void LightManager::Update() {
	time_ += 1.0f / 60.0f;

	// ゲーム側の指示（ImGui の手動操作がある場合はそちらを優先）
	bool enabled = manualOverride_ ? manualEnabled_ : lightsEnabled_;
	float nightGoal = manualOverride_ ? manualNight_ : nightTarget_;

	// スポットライトの点灯・消灯をなめらかに行う
	float fadeStep = 1.0f / (kFadeDuration * 60.0f);
	float diff = (enabled ? 1.0f : 0.0f) - lightFade_;
	if (diff > fadeStep) {
		diff = fadeStep;
	} else if (diff < -fadeStep) {
		diff = -fadeStep;
	}
	lightFade_ += diff;

	// ラスボスの発光をなめらかに点灯・消灯させる
	bossGlowFade_ += ((bossGlowActive_ ? 1.0f : 0.0f) - bossGlowFade_) * 0.03f;

	// 夜の度合いを目標値へなめらかに近づける（出現演出の間に朝から夜へ段々暗くなる）
	nightCurrent_ += (nightGoal - nightCurrent_) * kNightSmoothing;

	// 演出: 朝の太陽（メイン平行光源）をゆっくり円を描くように回転させて光と影の表情を変化させる
	//   左（日の出）➔ 上（昼）➔ 右（日の入り）➔ 下（地平線の下）と一周する。
	//   全てのライトグループの太陽の向きを同じにして、ゲーム内の全モデルのハイライト（光沢）の位置も太陽に合わせて動かす。
	//   太陽が高いほど明るく白い光、低いほど朝焼け・夕焼けの暖かい光、地平線の下では暗くなる（上昇と下降）
	if (isSunRotating_) {
		sunAngle_ += (std::numbers::pi_v<float> * 2.0f) / (kSunPeriodSeconds * 60.0f);
		if (sunAngle_ > std::numbers::pi_v<float> * 2.0f) {
			sunAngle_ -= std::numbers::pi_v<float> * 2.0f;
		}

		// 太陽の位置（画面上の円軌道）。sunY が太陽の高さ（1.0 = 真上、0.0 = 地平線、-1.0 = 真下）
		const float sunX = -std::cos(sunAngle_);
		const float sunY = std::sin(sunAngle_);

		// 光の進む向き = 太陽から見た向き（太陽の位置の逆）
		const Vector3 sunRay = {-sunX, -sunY, kSunDepth};
		for (LightSet& set : lightSets_) {
			set.dirLights[0].morning.direction = sunRay;
		}

		// 太陽の高さによる明るさ・色味・環境光の変化
		const float daylight = SmoothStep(-0.2f, 0.4f, sunY);
		sunBrightnessRate_ = 0.15f + 0.85f * daylight;
		sunAmbientRate_ = 0.8f + 0.2f * daylight;
		sunWarmRate_ = 1.0f - SmoothStep(0.05f, 0.55f, sunY);
	} else {
		sunBrightnessRate_ = 1.0f;
		sunAmbientRate_ = 1.0f;
		sunWarmRate_ = 0.0f;
	}

	TransferBuffer();
}

void LightManager::TransferBuffer() {
	for (size_t i = 0; i < lightSets_.size(); ++i) {
		ApplyLightSet(lightSets_[i], i == static_cast<size_t>(GroupType::kBoss));
	}
}

void LightManager::ApplyLightSet(LightSet& set, bool isBoss) {
	LightGroup* group = set.group.get();
	if (!group) {
		return;
	}

	const float night = nightCurrent_;
	// スポットライトは、点灯の進行度が 0 の間は無効にする
	const bool isSpotOn = lightFade_ > 0.0001f;

	// 環境光：朝の環境光から夜の環境光へ
	// ★ 太陽の高さによる環境光の変化は、朝のライティングの度合い（1.0 - night）に応じて反映する
	const float ambientRate = 1.0f + (sunAmbientRate_ - 1.0f) * (1.0f - night);
	group->SetAmbientColor(Scale(Lerp(set.ambientMorning, set.ambientNight, night), ambientRate));

	// 平行光源 0,1：朝の見た目から夜の見た目へ（向き・色・明るさ・彩度をまとめて補間）
	for (int i = 0; i < kUsableDirLights; ++i) {
		const DirState& d = set.dirLights[i];
		Vector3 morningColor = AdjustColor(d.morning.color, d.morning.brightness, d.morning.saturation);
		if (i == 0 && isSunRotating_) {
			// ★ メインの太陽: 低い位置では朝焼け・夕焼けの暖かい色に、地平線の下では暗くする
			morningColor = Scale(Lerp(morningColor, kSunWarmColor, sunWarmRate_ * 0.5f), sunBrightnessRate_);
		}
		Vector3 nightColor = AdjustColor(d.night.color, d.night.brightness, d.night.saturation);
		Vector3 direction = Lerp(d.morning.direction, d.night.direction, night);
		group->SetDirLightActive(i, d.active);
		group->SetDirLightDir(i, SafeDir(direction, {0.0f, -1.0f, 0.0f}, kDirLightSign));
		group->SetDirLightColor(i, Lerp(morningColor, nightColor, night));
	}

	// 平行光源 2 は無効にしたまま、シェーダーへ渡す設定値を color に格納する
	// （ObjPS.hlsl 側で dirLights[2].color を読む）
	//   x: Lambert(0) / HalfLambert(1)   y: 光沢の強さ   z: 光沢の鋭さ
	float currentGlossStrength = isBoss ? bossGlossStrength_ : glossStrength_;    // ★ ラスボス専用のハイライト強度を適用
	float currentGlossShininess = isBoss ? bossGlossShininess_ : glossShininess_; // ★ ラスボス専用のハイライト鋭さを適用

	group->SetDirLightActive(2, false);
	group->SetDirLightDir(2, {0.0f, -1.0f, 0.0f});
	group->SetDirLightColor(2, {static_cast<float>(lightType_), currentGlossStrength, currentGlossShininess});

	// 点光源
	for (int i = 0; i < static_cast<int>(set.pointLights.size()); ++i) {
		const PointState& p = set.pointLights[i];
		group->SetPointLightActive(i, p.active);
		group->SetPointLightColor(i, p.color);
		group->SetPointLightIntensity(i, p.intensity);
		group->SetPointLightPos(i, p.position);
		group->SetPointLightRadius(i, p.radius);
		group->SetPointLightDecay(i, p.decay);
	}

	// ★ ラスボスの自己発光：点光源 2 番の intensity をシェーダーへの受け渡しに使う（この点光源は常に無効のまま）
	// 夜になるほど強く光り、発狂中は脈打つように明滅する。ラスボス以外のグループは 0 を渡す
	{
		float glow = 0.0f;
		if (isBoss) {
			float pulseSpeed = isBossFrenzy_ ? 8.0f : 2.0f;
			float pulseAmp = isBossFrenzy_ ? 0.25f : 0.1f;
			glow = bossGlowStrength_ * bossGlowFade_ * (1.0f + 0.5f * night) * (1.0f + pulseAmp * std::sin(time_ * pulseSpeed));
		}
		group->SetPointLightActive(2, false);
		group->SetPointLightIntensity(2, glow);
	}

	// スポットライト：ゆらぎ（明滅）を加える。ラスボスの発狂中は激しく明滅、夜になるほど少し強くなる
	const bool isFrenzy = isBoss && isBossFrenzy_;
	const float pulseAmp = isFrenzy ? 0.2f : 0.08f;
	const float pulseSpeed = isFrenzy ? 10.0f : 2.5f;
	const float nightBoost = 1.0f + 0.3f * night;
	for (int i = 0; i < static_cast<int>(set.spotLights.size()); ++i) {
		const SpotState& s = set.spotLights[i];
		float pulse = 1.0f + pulseAmp * std::sin(time_ * pulseSpeed + static_cast<float>(i) * 1.7f);
		group->SetSpotLightActive(i, isSpotOn && s.active);
		group->SetSpotLightColor(i, Scale(s.color, lightFade_));
		group->SetSpotLightIntensity(i, s.intensity * pulse * nightBoost);
		group->SetSpotLightPos(i, s.position);
		group->SetSpotLightRadius(i, s.radius);
		group->SetSpotLightDir(i, SafeDir(s.direction, {0.0f, -1.0f, 0.0f}, kSpotLightSign));
		group->SetSpotLightDecay(i, s.decay);
		group->SetSpotLightFactorAngle(i, {ToEngineAngle(s.angleDeg.x), ToEngineAngle(s.angleDeg.y)});
	}

	// ダーティフラグが立っていれば定数バッファへ転送される
	group->Update();
}

// ★ 流星群攻撃中の finalBossCrystal_22 用ポイントライト（黄色）の設定
void LightManager::SetMeteorPointLights(bool active, const std::vector<KamataEngine::Vector3>& positions) {
	for (GroupType type : {GroupType::kWorld, GroupType::kBoss}) {
		PointState& p = GetSet(type).pointLights[1];
		if (active && !positions.empty()) {
			p.active = true;
			p.color = {1.0f, 0.90f, 0.15f}; // ★ 鮮やかな黄色
			p.intensity = 15.0f;            // ★ 強めの光で流星群を照らす
			p.radius = 22.0f;
			p.decay = 1.0f;

			// アクティブな流星群（finalBossCrystal_22）の位置にポイントライトを配置
			KamataEngine::Vector3 avgPos = {0.0f, 0.0f, 0.0f};
			for (const auto& pos : positions) {
				avgPos.x += pos.x;
				avgPos.y += pos.y;
				avgPos.z += pos.z;
			}
			float count = static_cast<float>(positions.size());
			p.position = {avgPos.x / count, avgPos.y / count, avgPos.z / count};
		} else {
			p.active = false;
			p.intensity = 0.0f;
		}
	}
}

void LightManager::ApplyToModel(Model* model, GroupType type) const {
	if (model) {
		model->SetLightGroup(GetSet(type).group.get());
	}
}

const LightGroup* LightManager::GetLightGroup(GroupType type) const { return GetSet(type).group.get(); }

void LightManager::FollowSpot(SpotState& spot, bool active, const Vector3& targetPos, const Vector3& offset) {
	spot.active = active;
	if (!active) {
		return;
	}
	// 対象の斜め上に置いて、対象に向ける
	spot.position = {targetPos.x + offset.x, targetPos.y + offset.y, targetPos.z + offset.z};
	spot.direction = {-offset.x, -offset.y, -offset.z};
}

void LightManager::SetPlayerSpot(bool active, const Vector3& targetPos) { FollowSpot(GetSet(GroupType::kWorld).spotLights[0], active, targetPos, spotOffset_); }

void LightManager::SetEnemySpot(bool active, const Vector3& targetPos) { FollowSpot(GetSet(GroupType::kWorld).spotLights[1], active, targetPos, spotOffset_); }

void LightManager::SetShieldEnemySpot(bool active, const Vector3& targetPos) { FollowSpot(GetSet(GroupType::kWorld).spotLights[2], active, targetPos, spotOffset_); }

// ImGui描画処理（ライトグループ 1 つ分）
void LightManager::DrawLightSetImGui(const char* name, LightSet& set, bool isWorld, int spotCount, const char* const* spotNames) {
#ifdef _DEBUG
	if (!ImGui::TreeNode(name)) {
		return;
	}

	// 影の暗さ調整（環境光の色・明るさ）
	ImGui::ColorEdit3("Ambient Color (Morning)", &set.ambientMorning.x);
	ImGui::ColorEdit3("Ambient Color (Night)", &set.ambientNight.x);

	// 平行光源 0, 1 の設定（朝用・夜用）
	for (int i = 0; i < kUsableDirLights; ++i) {
		std::string label = "Directional Light " + std::to_string(i);
		if (ImGui::TreeNode(label.c_str())) {
			DirState& d = set.dirLights[i];
			ImGui::Checkbox("Active", &d.active);

			for (int look = 0; look < 2; ++look) {
				DirLook& l = (look == 0) ? d.morning : d.night;
				if (ImGui::TreeNode((look == 0) ? "Morning (Before Boss)" : "Night (After Boss)")) {
					ImGui::ColorEdit3("Light Color", &l.color.x);
					ImGui::SliderFloat("Brightness", &l.brightness, 0.0f, 2.0f);
					ImGui::SliderFloat("Saturation", &l.saturation, 0.0f, 1.0f);
					// 光の進む向き (0,-1,0 = 真上から真下へ照らす)
					if (ImGui::DragFloat3("Direction (light ray)", &l.direction.x, 0.01f)) {
						if (isWorld && i == 0 && look == 0) {
							isSunRotating_ = false; // 手動調整したら自動回転を止める
						}
					}
					ImGui::TreePop();
				}
			}

			if (isWorld && i == 0) {
				ImGui::Checkbox("Sun Rotation", &isSunRotating_);
				ImGui::SliderAngle("Sun Angle (0:Left 90:Top 180:Right 270:Bottom)", &sunAngle_, 0.0f, 360.0f);
			}
			ImGui::TreePop();
		}
	}

	// 点光源 0 の設定
	if (isWorld && ImGui::TreeNode("Point Light 0")) {
		PointState& p = set.pointLights[0];
		ImGui::Checkbox("Active", &p.active);
		ImGui::ColorEdit3("Color", &p.color.x);
		ImGui::DragFloat("Intensity", &p.intensity, 0.1f, 0.0f, 20.0f);
		ImGui::DragFloat3("Position", &p.position.x, 0.1f);
		ImGui::DragFloat("Radius", &p.radius, 0.5f, 0.1f, 200.0f);
		ImGui::DragFloat("Decay", &p.decay, 0.05f, 0.0f, 10.0f);
		ImGui::TreePop();
	}

	// 追従スポットライトの設定（位置・向きは対象に追従して自動で決まる）
	for (int i = 0; i < spotCount; ++i) {
		if (ImGui::TreeNode(spotNames[i])) {
			SpotState& s = set.spotLights[i];
			ImGui::ColorEdit3("Color", &s.color.x);
			ImGui::DragFloat("Intensity", &s.intensity, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Radius", &s.radius, 0.5f, 0.1f, 200.0f);
			ImGui::DragFloat("Decay", &s.decay, 0.05f, 0.0f, 10.0f);
			ImGui::DragFloat2("Angle deg (Inner, Outer)", &s.angleDeg.x, 0.5f, 0.0f, 90.0f);
			ImGui::TreePop();
		}
	}

	ImGui::TreePop();
#else
	(void)name;
	(void)set;
	(void)isWorld;
	(void)spotCount;
	(void)spotNames;
#endif
}

// ImGui描画処理（変更した値は Update / TransferBuffer で毎フレーム反映される）
void LightManager::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::TreeNode("Lighting Settings")) {
		// シェーディングの切替
		int currentLighting = static_cast<int>(lightType_);
		if (ImGui::RadioButton("Lambert", &currentLighting, 0)) {
			lightType_ = LightingType::kLambert;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Half Lambert", &currentLighting, 1)) {
			lightType_ = LightingType::kHalfLambert;
		}

		// 光沢（全モデル共通）
		ImGui::SliderFloat("Gloss Strength", &glossStrength_, 0.0f, 2.0f);
		ImGui::SliderFloat("Gloss Shininess", &glossShininess_, 4.0f, 256.0f);

		// 光の演出の設定
		if (ImGui::TreeNode("Light Effect")) {
			ImGui::Checkbox("Manual Override", &manualOverride_);
			if (manualOverride_) {
				ImGui::Checkbox("Spot Lights Enabled", &manualEnabled_);
				ImGui::SliderFloat("Night Factor (0:Morning 1:Night)", &manualNight_, 0.0f, 1.0f);
			}
			ImGui::Text("Spot Fade: %.2f  Night: %.2f", lightFade_, nightCurrent_);
			ImGui::DragFloat3("Spot Offset", &spotOffset_.x, 0.1f);
			ImGui::SliderFloat("Boss Glow Strength", &bossGlowStrength_, 0.0f, 5.0f);

			// ★ ラスボス専用ハイライトの ImGui 調整項目
			ImGui::SliderFloat("Boss Gloss Strength", &bossGlossStrength_, 0.0f, 3.0f);
			ImGui::SliderFloat("Boss Gloss Shininess", &bossGlossShininess_, 2.0f, 128.0f);

			ImGui::TreePop();
		}

		static const char* const kWorldSpotNames[3] = {"Player Spot Light", "Enemy Spot Light", "ShieldEnemy Spot Light"};

		DrawLightSetImGui("World Lights", GetSet(GroupType::kWorld), true, 3, kWorldSpotNames);
		DrawLightSetImGui("Background Near Lights", GetSet(GroupType::kBackgroundNear), false, 0, nullptr);
		DrawLightSetImGui("Background Mid Lights", GetSet(GroupType::kBackgroundMid), false, 0, nullptr);
		DrawLightSetImGui("Background Far Lights", GetSet(GroupType::kBackgroundFar), false, 0, nullptr);
		DrawLightSetImGui("Boss Lights", GetSet(GroupType::kBoss), false, 0, nullptr);

		ImGui::TreePop();
	}
#endif
}
