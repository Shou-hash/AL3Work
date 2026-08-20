#include "Item.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "PlayerHp.h"
#include <algorithm>

void Item::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField) {
	model_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};

	// ★追加：放物線上に打ち上げる初期速度（上方向および前方向への移動）
	velocity_ = {0.02f, kJumpPowerY, 0.0f};
	isDead_ = false;
	isGrounded_ = false;
}

void Item::Update() {
	if (isDead_) {
		return;
	}

	// ★追加：アイテムの回転演出
	worldTransform_.rotation_.y += 0.05f;

	if (!isGrounded_) {
		// ★追加：重力を適用して放物線運動（落下）させる
		velocity_.y -= kGravity;

		worldTransform_.translation_.x += velocity_.x;
		worldTransform_.translation_.y += velocity_.y;
		worldTransform_.translation_.z += velocity_.z;

		// ★追加：地面（マップチップブロック）との衝突・着地判定
		if (mapChipField_) {
			KamataEngine::Vector3 checkPos = worldTransform_.translation_;
			checkPos.y -= 0.3f;
			MapChipType chipType = mapChipField_->GetMapChipTypeByPosition(checkPos);
			if (chipType == MapChipType::kBlock && velocity_.y < 0.0f) {
				isGrounded_ = true;
				velocity_ = {0.0f, 0.0f, 0.0f};

				// ブロックの上面に着地補正
				MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(checkPos);
				KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);
				worldTransform_.translation_.y = blockPos.y + 0.8f;
			}
		}
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Item::Draw() {
	if (isDead_) {
		return;
	}

	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_);
	}
}

void Item::OnCollision(Player* player) {
	(void)player;
	isDead_ = true;
}

Item::AABB Item::GetAABB() const {
	AABB aabb;
	const auto& pos = worldTransform_.translation_;
	aabb.min = {pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f};
	aabb.max = {pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f};
	return aabb;
}