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

// 光の演出が全点灯するまでの時間（秒）
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

Vector3 Mul(const Vector3& a, const Vector3& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

Vector3 Scale(const Vector3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }

// 彩度と明るさを調整した色を返す
Vector3 AdjustColor(const Vector3& color, float brightness, float saturation) {
	float gray = color.x * 0.299f + color.y * 0.587f + color.z * 0.114f;
	Vector3 gray3 = {gray, gray, gray};
	return Scale(Lerp(gray3, color, saturation), brightness);
}

void InitDirLight(LightManager::DirState& d, const Vector3& direction, const Vector3& color, bool active, float brightness, float saturation) {
	d.direction = direction;
	d.color = color;
	d.active = active;
	d.brightness = brightness;
	d.saturation = saturation;
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

void LightManager::Initialize() {
	// エンジン標準のライトグループを生成（種類ごとに 1 つ）
	for (auto& set : lightSets_) {
		set.group.reset(LightGroup::Create());
	}

	lightType_ = LightingType::kHalfLambert;
	lightsEnabled_ = false;
	lightFade_ = 0.0f;
	nightTarget_ = 0.0f;
	nightCurrent_ = 0.0f;
	isBossFrenzy_ = false;
	time_ = 0.0f;

	// =========================================================
	// 通常（ブロック・プレイヤー・敵など）
	// =========================================================
	LightSet& worldLights = GetSet(GroupType::kWorld);
	worldLights.ambientColor = {0.3f, 0.3f, 0.3f};
	// メインの平行光源（光の進む向き。上から斜めに差し込む）
	InitDirLight(worldLights.dirLights[0], {-0.5f, -1.0f, 0.5f}, {1.0f, 0.95f, 0.85f}, true, 1.0f, 1.0f);
	InitDirLight(worldLights.dirLights[1], {0.0f, -1.0f, 0.3f}, {1.0f, 1.0f, 1.0f}, false, 1.0f, 1.0f);
	InitRedSpot(worldLights.spotLights[0], 2.0f, 25.0f); // プレイヤー
	InitRedSpot(worldLights.spotLights[1], 2.0f, 25.0f); // 普通敵
	InitRedSpot(worldLights.spotLights[2], 2.0f, 25.0f); // 盾敵

	// =========================================================
	// 奥行き背景（中景）：やや明るく、彩度はそこそこ
	// =========================================================
	LightSet& midLights = GetSet(GroupType::kBackgroundMid);
	midLights.ambientColor = {0.25f, 0.25f, 0.27f};
	InitDirLight(midLights.dirLights[0], {-0.3f, -0.8f, 0.8f}, {1.0f, 0.85f, 0.65f}, true, 0.8f, 0.7f);
	InitDirLight(midLights.dirLights[1], {0.0f, -1.0f, 0.3f}, {1.0f, 1.0f, 1.0f}, false, 1.0f, 1.0f);

	// =========================================================
	// 奥行き背景（遠景・一番奥）：一番暗く、彩度も一番低い
	// =========================================================
	LightSet& farLights = GetSet(GroupType::kBackgroundFar);
	farLights.ambientColor = {0.12f, 0.12f, 0.14f};
	InitDirLight(farLights.dirLights[0], {-0.3f, -0.8f, 0.8f}, {0.7f, 0.75f, 0.9f}, true, 0.35f, 0.2f);
	InitDirLight(farLights.dirLights[1], {0.0f, -1.0f, 0.3f}, {1.0f, 1.0f, 1.0f}, false, 1.0f, 1.0f);

	// =========================================================
	// ラスボス
	// =========================================================
	LightSet& bossLights = GetSet(GroupType::kBoss);
	bossLights.ambientColor = {0.2f, 0.2f, 0.22f};
	InitDirLight(bossLights.dirLights[0], {0.0f, -0.6f, 1.0f}, {0.9f, 0.8f, 0.85f}, true, 0.7f, 0.6f);
	InitDirLight(bossLights.dirLights[1], {0.0f, -1.0f, 0.3f}, {1.0f, 1.0f, 1.0f}, false, 1.0f, 1.0f);
	InitRedSpot(bossLights.spotLights[0], 2.5f, 40.0f);

	TransferBuffer();
}

void LightManager::Update() {
	time_ += 1.0f / 60.0f;

	// ゲーム側の指示（ImGui の手動操作がある場合はそちらを優先）
	bool enabled = manualOverride_ ? manualEnabled_ : lightsEnabled_;
	float nightGoal = manualOverride_ ? manualNight_ : nightTarget_;

	// 点灯・消灯をなめらかに行う
	float fadeStep = 1.0f / (kFadeDuration * 60.0f);
	float diff = (enabled ? 1.0f : 0.0f) - lightFade_;
	if (diff > fadeStep) {
		diff = fadeStep;
	} else if (diff < -fadeStep) {
		diff = -fadeStep;
	}
	lightFade_ += diff;

	// 夜の度合いを目標値へなめらかに近づける（出現演出の間に段々暗くなる）
	nightCurrent_ += (nightGoal - nightCurrent_) * kNightSmoothing;

	// 演出: 太陽（メイン平行光源）をゆっくり回転させて光と影の表情を変化させる
	if (isSunRotating_) {
		sunAngle_ += 0.005f;
		if (sunAngle_ > std::numbers::pi_v<float> * 2.0f) {
			sunAngle_ -= std::numbers::pi_v<float> * 2.0f;
		}
		LightSet& worldLights = GetSet(GroupType::kWorld);
		worldLights.dirLights[0].direction.x = std::cos(sunAngle_);
		worldLights.dirLights[0].direction.z = std::sin(sunAngle_);
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

	const float fade = lightFade_;
	const float night = nightCurrent_;
	// 光の演出が完全に消えている間は、全てのライトを無効にする
	const bool isOn = fade > 0.0001f;

	// 環境光：消灯中は一様な環境光のみ、点灯するにつれて昼の環境光へ、夜になるほど暗い青へ
	Vector3 nightAmbient = Mul(set.ambientColor, nightAmbientMul_);
	Vector3 litAmbient = Lerp(set.ambientColor, nightAmbient, night);
	group->SetAmbientColor(Lerp(unlitAmbient_, litAmbient, fade));

	// 平行光源 0,1
	for (int i = 0; i < kUsableDirLights; ++i) {
		const DirState& d = set.dirLights[i];
		Vector3 dayColor = AdjustColor(d.color, d.brightness, d.saturation);
		Vector3 nightColor = Mul(dayColor, nightDirMul_);
		group->SetDirLightActive(i, isOn && d.active);
		group->SetDirLightDir(i, SafeDir(d.direction, {0.0f, -1.0f, 0.0f}, kDirLightSign));
		group->SetDirLightColor(i, Scale(Lerp(dayColor, nightColor, night), fade));
	}

	// 平行光源 2 は無効にしたまま、color.x に Lambert(0) / HalfLambert(1) の切替値を格納する
	// （ObjPS.hlsl 側で dirLights[2].color.x を読んで切り替える）
	group->SetDirLightActive(2, false);
	group->SetDirLightDir(2, {0.0f, -1.0f, 0.0f});
	group->SetDirLightColor(2, {static_cast<float>(lightType_), 0.0f, 0.0f});

	// 点光源
	for (int i = 0; i < static_cast<int>(set.pointLights.size()); ++i) {
		const PointState& p = set.pointLights[i];
		group->SetPointLightActive(i, isOn && p.active);
		group->SetPointLightColor(i, Scale(p.color, fade));
		group->SetPointLightIntensity(i, p.intensity);
		group->SetPointLightPos(i, p.position);
		group->SetPointLightRadius(i, p.radius);
		group->SetPointLightDecay(i, p.decay);
	}

	// スポットライト：ゆらぎ（明滅）を加える。ラスボスの発狂中は激しく明滅、夜になるほど少し強くなる
	const bool isFrenzy = isBoss && isBossFrenzy_;
	const float pulseAmp = isFrenzy ? 0.35f : 0.08f;
	const float pulseSpeed = isFrenzy ? 10.0f : 2.5f;
	const float nightBoost = 1.0f + 0.5f * night;
	for (int i = 0; i < static_cast<int>(set.spotLights.size()); ++i) {
		const SpotState& s = set.spotLights[i];
		float pulse = 1.0f + pulseAmp * std::sin(time_ * pulseSpeed + static_cast<float>(i) * 1.7f);
		group->SetSpotLightActive(i, isOn && s.active);
		group->SetSpotLightColor(i, Scale(s.color, fade));
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

void LightManager::SetBossSpot(bool active, const Vector3& targetPos) { FollowSpot(GetSet(GroupType::kBoss).spotLights[0], active, targetPos, bossSpotOffset_); }

// ImGui描画処理（ライトグループ 1 つ分）
void LightManager::DrawLightSetImGui(const char* name, LightSet& set, bool isWorld, int spotCount, const char* const* spotNames) {
#ifdef _DEBUG
	if (!ImGui::TreeNode(name)) {
		return;
	}

	// 影の暗さ調整（環境光の色・明るさ）
	ImGui::ColorEdit3("Shadow Darkness (Ambient Color)", &set.ambientColor.x);

	// 平行光源 0, 1 の設定
	for (int i = 0; i < kUsableDirLights; ++i) {
		std::string label = "Directional Light " + std::to_string(i);
		if (ImGui::TreeNode(label.c_str())) {
			DirState& d = set.dirLights[i];
			ImGui::Checkbox("Active", &d.active);
			ImGui::ColorEdit3("Light Color", &d.color.x);
			ImGui::SliderFloat("Brightness", &d.brightness, 0.0f, 2.0f);
			ImGui::SliderFloat("Saturation", &d.saturation, 0.0f, 1.0f);
			// 光の進む向き (0,-1,0 = 真上から真下へ照らす)
			if (ImGui::DragFloat3("Direction (light ray)", &d.direction.x, 0.01f)) {
				if (isWorld && i == 0) {
					isSunRotating_ = false; // 手動調整したら自動回転を止める
				}
			}
			if (isWorld && i == 0) {
				ImGui::Checkbox("Sun Rotation", &isSunRotating_);
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

		// 光の演出の設定
		if (ImGui::TreeNode("Light Effect")) {
			ImGui::Checkbox("Manual Override", &manualOverride_);
			if (manualOverride_) {
				ImGui::Checkbox("Lights Enabled", &manualEnabled_);
				ImGui::SliderFloat("Night Factor", &manualNight_, 0.0f, 1.0f);
			}
			ImGui::Text("Light Fade: %.2f  Night: %.2f", lightFade_, nightCurrent_);
			ImGui::ColorEdit3("Unlit Ambient (Lights Off)", &unlitAmbient_.x);
			ImGui::ColorEdit3("Night Ambient Mul", &nightAmbientMul_.x);
			ImGui::ColorEdit3("Night Light Mul", &nightDirMul_.x);
			ImGui::DragFloat3("Spot Offset", &spotOffset_.x, 0.1f);
			ImGui::DragFloat3("Boss Spot Offset", &bossSpotOffset_.x, 0.1f);
			ImGui::TreePop();
		}

		static const char* const kWorldSpotNames[3] = {"Player Spot Light", "Enemy Spot Light", "ShieldEnemy Spot Light"};
		static const char* const kBossSpotNames[1] = {"Boss Spot Light"};

		DrawLightSetImGui("World Lights", GetSet(GroupType::kWorld), true, 3, kWorldSpotNames);
		DrawLightSetImGui("Background Mid Lights", GetSet(GroupType::kBackgroundMid), false, 0, nullptr);
		DrawLightSetImGui("Background Far Lights", GetSet(GroupType::kBackgroundFar), false, 0, nullptr);
		DrawLightSetImGui("Boss Lights", GetSet(GroupType::kBoss), false, 1, kBossSpotNames);

		ImGui::TreePop();
	}
#endif
}
