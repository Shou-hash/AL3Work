#include "CrystalArrow.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>

namespace {
// ★ 矢印の調整用パラメータ
constexpr float kArrowBaseAngle = 0.0f;    // モデルの最初の向き（右 = 0.0）。モデルが左向きの場合は π にする
constexpr float kArrowBaseScale = 0.9f;    // 矢印の基本の大きさ
constexpr float kArrowRadius = 2.4f;       // プレイヤーの中心から矢印までの距離
constexpr float kArrowBobDistance = 0.35f; // 脈打つときに外側へ動く距離
constexpr float kArrowZOffset = -0.6f;     // プレイヤーより少しカメラ側へ出して、体に隠れないようにする
constexpr float kHideDistance = 3.0f;      // クリスタルがこの距離より近いときは、矢印を消す（すぐそばに見えているため）
constexpr float kAppearTime = 0.25f;       // 出現にかかる時間（秒）
constexpr float kDisappearTime = 0.15f;    // 消えるのにかかる時間（秒）

// 最後に少し行き過ぎてから戻る（ポンッと出る）イージング
float EaseOutBack(float t) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	const float u = t - 1.0f;
	return 1.0f + c3 * u * u * u + c1 * u * u;
}
} // namespace

void CrystalArrow::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {
	model_ = model;
	camera_ = camera;
	worldTransform_.Initialize();
	timer_ = 0.0f;
	appear_ = 0.0f;
	wasVisible_ = false;
	isDrawEnabled_ = false;
}

void CrystalArrow::Update(bool active, const KamataEngine::Vector3& playerCenter, const KamataEngine::Vector3& targetPos) {
	const float dt = 1.0f / 60.0f;
	timer_ += dt;

	const float dx = targetPos.x - playerCenter.x;
	const float dy = targetPos.y - playerCenter.y;
	const float distance = std::sqrt(dx * dx + dy * dy);

	// クリスタルが遠いときだけ表示する
	const bool visible = active && distance > kHideDistance;
	if (visible) {
		appear_ = (std::min)(1.0f, appear_ + dt / kAppearTime);
	} else {
		appear_ = (std::max)(0.0f, appear_ - dt / kDisappearTime);
	}
	wasVisible_ = visible;

	isDrawEnabled_ = appear_ > 0.0f;
	if (!isDrawEnabled_) {
		return;
	}

	// 向き: プレイヤーからクリスタルへ（Z軸まわりの回転）
	const float angle = std::atan2(dy, dx) + kArrowBaseAngle;
	const float dirX = distance > 0.0001f ? dx / distance : 1.0f;
	const float dirY = distance > 0.0001f ? dy / distance : 0.0f;

	// 脈打ち: クリスタルに近いほど速く脈打つ（遠いときはゆっくり）
	const float urgency = 1.0f - std::clamp((distance - kHideDistance) / 25.0f, 0.0f, 1.0f);
	const float pulse = std::sin(timer_ * (6.0f + 8.0f * urgency)) * 0.5f + 0.5f;

	// ★ Scale の演出: 出現時はポンッと拡大し、指している間は矢印の向きに伸び縮みする（縦は逆に少し縮む）
	const float pop = EaseOutBack(appear_);
	const float along = kArrowBaseScale * (1.0f + 0.30f * pulse) * pop;
	const float across = kArrowBaseScale * (1.0f - 0.10f * pulse) * pop;
	worldTransform_.scale_ = {along, across, across};

	// 位置: プレイヤーの周りの円周上。脈打つたびに少し外側へ動く
	const float radius = kArrowRadius + kArrowBobDistance * pulse;
	worldTransform_.translation_ = {playerCenter.x + dirX * radius, playerCenter.y + dirY * radius, playerCenter.z + kArrowZOffset};
	worldTransform_.rotation_ = {0.0f, 0.0f, angle};

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void CrystalArrow::Draw() {
	if (!isDrawEnabled_ || !model_ || !camera_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_);
}
