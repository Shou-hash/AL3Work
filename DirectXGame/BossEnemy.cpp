#include "BossEnemy.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

BossEnemy::~BossEnemy() {
	delete spriteHpBG_;
	delete spriteHpBar_;
}

KamataEngine::Matrix4x4 BossEnemy::MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b) {
	KamataEngine::Matrix4x4 r = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float sum = 0.0f;
			for (int k = 0; k < 4; ++k) {
				sum += a.m[i][k] * b.m[k][j];
			}
			r.m[i][j] = sum;
		}
	}
	return r;
}

void BossEnemy::Initialize(
    KamataEngine::Model* modelBody, KamataEngine::Model* modelHead, KamataEngine::Model* modelLeft, KamataEngine::Model* modelRight, KamataEngine::Camera* camera,
    const KamataEngine::Vector3& position, MapChipField* mapChipField) {

	modelBody_ = modelBody;
	modelHead_ = modelHead;
	modelLeft_ = modelLeft;
	modelRight_ = modelRight;
	camera_ = camera;
	mapChipField_ = mapChipField;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f; // 左向きに配置
	worldTransform_.scale_ = {2.5f, 2.5f, 2.5f};                     // ボス用に大きめのスケール設定

	worldTransformBody_.Initialize();
	worldTransformHead_.Initialize();
	worldTransformLeft_.Initialize();
	worldTransformRight_.Initialize();

	animTimer_ = 0.0f;
	hp_ = 5;
	maxHp_ = 5;
	damageCooldown_ = 0.0f;
	isDead_ = false;
	isCollisionDisabled_ = false;
	lrDirection_ = BossEnemyLRDirection::kLeft;

	// ★ 攻撃状態初期化
	state_ = BossState::kWalk;
	attackCooldown_ = 0.0f;
	attackTimer_ = 0.0f;

	// スプライトの生成
	textureHandle_ = KamataEngine::TextureManager::Load("white1x1.png");
	if (spriteHpBG_ == nullptr) {
		spriteHpBG_ = KamataEngine::Sprite::Create(textureHandle_, {0.0f, 0.0f});
	}
	if (spriteHpBar_ == nullptr) {
		spriteHpBar_ = KamataEngine::Sprite::Create(textureHandle_, {0.0f, 0.0f});
	}
}

void BossEnemy::Update() {
	if (isDead_) {
		return;
	}

	// ダメージクールダウンタイマーの更新
	if (damageCooldown_ > 0.0f) {
		damageCooldown_ -= 1.0f / 60.0f;
		if (damageCooldown_ < 0.0f) {
			damageCooldown_ = 0.0f;
		}
	}

	// ★ 攻撃クールダウンタイマーの更新
	if (attackCooldown_ > 0.0f) {
		attackCooldown_ -= 1.0f / 60.0f;
		if (attackCooldown_ < 0.0f) {
			attackCooldown_ = 0.0f;
		}
	}

	// ★ ボスの行動状態処理
	switch (state_) {
	case BossState::kWalk: {
		// 両手の座標と回転の初期化（元の位置へ補間復帰）
		worldTransformLeft_.translation_.x += (0.0f - worldTransformLeft_.translation_.x) * 0.1f;
		worldTransformLeft_.translation_.y += (0.0f - worldTransformLeft_.translation_.y) * 0.1f;
		worldTransformLeft_.translation_.z += (0.0f - worldTransformLeft_.translation_.z) * 0.1f;
		worldTransformRight_.translation_.x += (0.0f - worldTransformRight_.translation_.x) * 0.1f;
		worldTransformRight_.translation_.y += (0.0f - worldTransformRight_.translation_.y) * 0.1f;
		worldTransformRight_.translation_.z += (0.0f - worldTransformRight_.translation_.z) * 0.1f;

		worldTransformLeft_.rotation_.z += (0.0f - worldTransformLeft_.rotation_.z) * 0.1f;
		worldTransformRight_.rotation_.z += (0.0f - worldTransformRight_.rotation_.z) * 0.1f;

		// プレイヤーとの距離判定による攻撃移行
		if (player_ && !player_->IsDead() && attackCooldown_ <= 0.0f) {
			float dx = player_->GetWorldTransform().translation_.x - worldTransform_.translation_.x;
			float dy = player_->GetWorldTransform().translation_.y - worldTransform_.translation_.y;

			// 近接検知（指定範囲内にプレイヤーが存在する場合）
			if (std::abs(dx) <= kAttackSearchDistanceX && std::abs(dy) <= kAttackSearchDistanceY) {
				// ★ プレイヤーの左右位置を判別し、向きを確定して攻撃準備へ遷移
				if (dx > 0.0f) {
					lrDirection_ = BossEnemyLRDirection::kRight;
				} else {
					lrDirection_ = BossEnemyLRDirection::kLeft;
				}

				// ★ 攻撃種類の分岐判定（距離またはランダムで「突進攻撃」か「両手叩きつけ攻撃」を選択）
				if (std::abs(dx) <= 3.0f) {
					state_ = BossState::kAttackSlamCharge;
				} else {
					state_ = (rand() % 2 == 0) ? BossState::kAttackCharge : BossState::kAttackSlamCharge;
				}

				attackTimer_ = 0.0f;
				break;
			}
		}

		// ★ 足場の端での移動反転処理
		if (mapChipField_) {
			float checkOffsetX = (lrDirection_ == BossEnemyLRDirection::kLeft) ? -1.0f : 1.0f;

			KamataEngine::Vector3 frontPos = worldTransform_.translation_;
			frontPos.x += checkOffsetX;

			KamataEngine::Vector3 frontDownPos = frontPos;
			frontDownPos.y -= 1.0f; // ボスの体格に合わせた足元座標

			MapChipType frontType = mapChipField_->GetMapChipTypeByPosition(frontPos);
			MapChipType frontDownType = mapChipField_->GetMapChipTypeByPosition(frontDownPos);

			if (frontType == MapChipType::kBlock || frontDownType == MapChipType::kBlank) {
				// 反転処理
				if (lrDirection_ == BossEnemyLRDirection::kLeft) {
					lrDirection_ = BossEnemyLRDirection::kRight;
				} else {
					lrDirection_ = BossEnemyLRDirection::kLeft;
				}
			}
		}

		// ★ 通常移動
		float moveSpeed = (lrDirection_ == BossEnemyLRDirection::kLeft) ? -kWalkspeed : kWalkspeed;
		worldTransform_.translation_.x += moveSpeed;

		// 簡易的な待機スイングアニメーション
		animTimer_ += 1.0f / 60.0f;
		float swing = std::sin(animTimer_ * 3.0f) * 0.3f;
		worldTransformLeft_.rotation_.x = swing;
		worldTransformRight_.rotation_.x = -swing;

		break;
	}

	case BossState::kAttackCharge: {
		// ★ 突進攻撃溜め動作（肩の位置translation_は動かさず、回転のみで後ろへ引き構える）
		attackTimer_ += 1.0f / 60.0f;

		worldTransformLeft_.translation_ = {0.0f, 0.0f, 0.0f};
		worldTransformRight_.translation_ = {0.0f, 0.0f, 0.0f};

		worldTransformLeft_.rotation_.x = -0.8f;
		worldTransformRight_.rotation_.x = -0.8f;
		worldTransformLeft_.rotation_.z = 0.0f;
		worldTransformRight_.rotation_.z = 0.0f;

		if (attackTimer_ >= kChargeDuration) {
			state_ = BossState::kAttackDash;
			attackTimer_ = 0.0f;
		}
		break;
	}

	case BossState::kAttackDash: {
		// ★ 確定した向きのまま直線突進移動
		attackTimer_ += 1.0f / 60.0f;

		float moveSpeed = (lrDirection_ == BossEnemyLRDirection::kLeft) ? -kDashSpeed : kDashSpeed;
		worldTransform_.translation_.x += moveSpeed;

		// 壁および崖への押し戻し処理（突き抜け・落下防止）
		if (mapChipField_) {
			float checkOffsetX = (lrDirection_ == BossEnemyLRDirection::kLeft) ? -1.2f : 1.2f;
			KamataEngine::Vector3 frontPos = worldTransform_.translation_;
			frontPos.x += checkOffsetX;

			KamataEngine::Vector3 frontDownPos = frontPos;
			frontDownPos.y -= 1.0f;

			MapChipType frontType = mapChipField_->GetMapChipTypeByPosition(frontPos);
			MapChipType frontDownType = mapChipField_->GetMapChipTypeByPosition(frontDownPos);

			if (frontType == MapChipType::kBlock || frontDownType == MapChipType::kBlank) {
				worldTransform_.translation_.x -= moveSpeed;
				state_ = BossState::kAttackRecoil;
				attackTimer_ = 0.0f;
			}
		}

		// ★ 突進攻撃アニメーション（肩の座標translation_は動かさず、X軸回転だけで前方に突き出す）
		worldTransformLeft_.translation_ = {0.0f, 0.0f, 0.0f};
		worldTransformRight_.translation_ = {0.0f, 0.0f, 0.0f};

		worldTransformLeft_.rotation_.x = 1.2f;
		worldTransformRight_.rotation_.x = 1.2f;
		worldTransformLeft_.rotation_.z = 0.0f;
		worldTransformRight_.rotation_.z = 0.0f;

		if (attackTimer_ >= kDashDuration) {
			state_ = BossState::kAttackRecoil;
			attackTimer_ = 0.0f;
		}
		break;
	}

	case BossState::kAttackSlamCharge: {
		// ★ 両手叩きつけ溜め動作（両手を高く振り上げる）
		attackTimer_ += 1.0f / 60.0f;
		float rate = (std::min)(1.0f, attackTimer_ / kSlamChargeDuration);

		worldTransformLeft_.rotation_.x = -1.2f * rate;
		worldTransformRight_.rotation_.x = -1.2f * rate;
		worldTransformLeft_.rotation_.z = -0.3f * rate; // 外側へ開くZ回転
		worldTransformRight_.rotation_.z = 0.3f * rate;

		worldTransformLeft_.translation_.y = 1.8f * rate; // 高く溜める
		worldTransformRight_.translation_.y = 1.8f * rate;
		worldTransformLeft_.translation_.z = -0.5f * rate;
		worldTransformRight_.translation_.z = -0.5f * rate;

		if (attackTimer_ >= kSlamChargeDuration) {
			state_ = BossState::kAttackSlam;
			attackTimer_ = 0.0f;
		}
		break;
	}

	case BossState::kAttackSlam: {
		// ★ 両手叩きつけ攻撃動作（手先をプレイヤー方向へ突き出し、内側へひねりながら振り下ろす）
		attackTimer_ += 1.0f / 60.0f;

		worldTransformLeft_.rotation_.x = 0.4f;
		worldTransformRight_.rotation_.x = 0.4f;
		worldTransformLeft_.rotation_.z = 0.5f; // 内側へ絞り込むZ回転
		worldTransformRight_.rotation_.z = -0.5f;

		worldTransformLeft_.translation_.y = 0.2f; // 地面付近の高さ
		worldTransformRight_.translation_.y = 0.2f;
		worldTransformLeft_.translation_.z = 1.8f; // 手先が届くよう前方（Z方向）へしっかり伸ばす
		worldTransformRight_.translation_.z = 1.8f;

		if (attackTimer_ >= kSlamDuration) {
			state_ = BossState::kAttackRecoil;
			attackTimer_ = 0.0f;
		}
		break;
	}

	case BossState::kAttackRecoil: {
		// ★ 攻撃直後のスキ（硬直・腕の座標と回転を復帰させる）
		attackTimer_ += 1.0f / 60.0f;

		worldTransformLeft_.rotation_.x *= 0.8f;
		worldTransformRight_.rotation_.x *= 0.8f;
		worldTransformLeft_.rotation_.z *= 0.8f;
		worldTransformRight_.rotation_.z *= 0.8f;

		worldTransformLeft_.translation_.x *= 0.8f;
		worldTransformLeft_.translation_.y *= 0.8f;
		worldTransformLeft_.translation_.z *= 0.8f;
		worldTransformRight_.translation_.x *= 0.8f;
		worldTransformRight_.translation_.y *= 0.8f;
		worldTransformRight_.translation_.z *= 0.8f;

		if (attackTimer_ >= kRecoilDuration) {
			state_ = BossState::kWalk;
			attackCooldown_ = kAttackCooldownDuration; // 攻撃クールタイム発生
			attackTimer_ = 0.0f;
		}
		break;
	}
	}

	// ★ 移動と向きの更新（攻撃中も向きを維持）
	float baseAngleY = (lrDirection_ == BossEnemyLRDirection::kLeft) ? -std::numbers::pi_v<float> / 2.0f : std::numbers::pi_v<float> / 2.0f;
	worldTransform_.rotation_.y = baseAngleY;

	// ルート行列の計算
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 各ローカル行列
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);

	KamataEngine::Vector3 centerOffset = {0.0f, -0.5f, 0.0f};

	// 左部位
	KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformLeft_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 preTranslateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
	KamataEngine::Vector3 finalTranslationLeft = {
	    worldTransformLeft_.translation_.x + centerOffset.x, worldTransformLeft_.translation_.y + centerOffset.y, worldTransformLeft_.translation_.z + centerOffset.z};
	KamataEngine::Matrix4x4 postTranslateLeft = MakeAffineMatrix(worldTransformLeft_.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationLeft);
	KamataEngine::Matrix4x4 localMatrixLeft = MultiplyMatrix(postTranslateLeft, MultiplyMatrix(rotateLeft, preTranslateLeft));

	// 右部位
	KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformRight_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 preTranslateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
	KamataEngine::Vector3 finalTranslationRight = {
	    worldTransformRight_.translation_.x + centerOffset.x, worldTransformRight_.translation_.y + centerOffset.y, worldTransformRight_.translation_.z + centerOffset.z};
	KamataEngine::Matrix4x4 postTranslateRight = MakeAffineMatrix(worldTransformRight_.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationRight);
	KamataEngine::Matrix4x4 localMatrixRight = MultiplyMatrix(postTranslateRight, MultiplyMatrix(rotateRight, preTranslateRight));

	// 親子関係合成
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform_.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();

	// ★HPバーの表示座標設定（ワールド座標からスクリーン座標へ変換）
	if (camera_ && spriteHpBG_ && spriteHpBar_) {
		KamataEngine::Vector3 hpWorldPos = worldTransform_.translation_;
		hpWorldPos.y += 3.2f; // ボスの頭上に配置

		KamataEngine::Matrix4x4 matViewProj = MultiplyMatrix(camera_->matView, camera_->matProjection);

		float x = hpWorldPos.x * matViewProj.m[0][0] + hpWorldPos.y * matViewProj.m[1][0] + hpWorldPos.z * matViewProj.m[2][0] + matViewProj.m[3][0];
		float y = hpWorldPos.x * matViewProj.m[0][1] + hpWorldPos.y * matViewProj.m[1][1] + hpWorldPos.z * matViewProj.m[2][1] + matViewProj.m[3][1];
		float w = hpWorldPos.x * matViewProj.m[0][3] + hpWorldPos.y * matViewProj.m[1][3] + hpWorldPos.z * matViewProj.m[2][3] + matViewProj.m[3][3];

		if (w != 0.0f) {
			x /= w;
			y /= w;
		}

		float screenX = (x + 1.0f) * 0.5f * 1280.0f;
		float screenY = (1.0f - y) * 0.5f * 720.0f;

		float maxBarWidth = 120.0f;
		float barHeight = 12.0f;
		float currentBarWidth = maxBarWidth * (std::max)(0.0f, static_cast<float>(hp_) / static_cast<float>(maxHp_));

		// 背景バーの設定
		spriteHpBG_->SetPosition({screenX - maxBarWidth * 0.5f, screenY - barHeight * 0.5f});
		spriteHpBG_->SetSize({maxBarWidth, barHeight});
		spriteHpBG_->SetColor({0.2f, 0.2f, 0.2f, 0.8f});

		// HP残量バーの設定
		spriteHpBar_->SetPosition({screenX - maxBarWidth * 0.5f, screenY - barHeight * 0.5f});
		spriteHpBar_->SetSize({currentBarWidth, barHeight});
		spriteHpBar_->SetColor({1.0f, 0.2f, 0.2f, 1.0f});
	}
}

void BossEnemy::OnCollision(Player* player) {
	if (isDead_ || isCollisionDisabled_) {
		return;
	}

	// 被ダメージ後のクールタイム中、または攻撃状態中（無敵）は攻撃を受けない
	if (damageCooldown_ > 0.0f || state_ != BossState::kWalk) {
		return;
	}

	if (player && player->IsAttacking()) {
		hp_--;
		damageCooldown_ = 0.5f; // ダメージ発生後の無敵時間

		if (hp_ <= 0) {
			OnDead();
		}
	}
}

void BossEnemy::OnDead() {
	hp_ = 0;
	isDead_ = true;
	isCollisionDisabled_ = true;
}

void BossEnemy::Draw() {
	if (isDead_ || !camera_) {
		return;
	}

	if (modelBody_ && modelHead_ && modelLeft_ && modelRight_) {
		modelBody_->Draw(worldTransformBody_, *camera_);
		modelHead_->Draw(worldTransformHead_, *camera_);
		modelLeft_->Draw(worldTransformLeft_, *camera_);
		modelRight_->Draw(worldTransformRight_, *camera_);
	}

	// HPバーの描画
	if (spriteHpBG_ && spriteHpBar_ && hp_ > 0) {
		KamataEngine::Sprite::PreDraw();
		spriteHpBG_->Draw();
		spriteHpBar_->Draw();
		KamataEngine::Sprite::PostDraw();
	}
}

BaseEnemy::AABB BossEnemy::GetAABB() const {
	if (isCollisionDisabled_ || isDead_) {
		return AABB{
		    {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f}
        };
	}

	AABB aabb;
	const auto& pos = worldTransform_.translation_;
	aabb.min = {pos.x - 1.2f, pos.y - 1.2f, pos.z - 1.2f};
	aabb.max = {pos.x + 1.2f, pos.y + 1.2f, pos.z + 1.2f};

	// ★ 叩きつけ攻撃中は、伸ばした両手のワールド位置も含めてAABBを拡張する
	if (state_ == BossState::kAttackSlam) {
		KamataEngine::Vector3 leftPos = {worldTransformLeft_.matWorld_.m[3][0], worldTransformLeft_.matWorld_.m[3][1], worldTransformLeft_.matWorld_.m[3][2]};
		KamataEngine::Vector3 rightPos = {worldTransformRight_.matWorld_.m[3][0], worldTransformRight_.matWorld_.m[3][1], worldTransformRight_.matWorld_.m[3][2]};

		float handSize = 1.5f; // 手パーツの当たり判定半径

		aabb.min.x = (std::min)({aabb.min.x, leftPos.x - handSize, rightPos.x - handSize});
		aabb.max.x = (std::max)({aabb.max.x, leftPos.x + handSize, rightPos.x + handSize});
		aabb.min.y = (std::min)({aabb.min.y, leftPos.y - handSize, rightPos.y - handSize});
		aabb.max.y = (std::max)({aabb.max.y, leftPos.y + handSize, rightPos.y + handSize});
		aabb.min.z = (std::min)({aabb.min.z, leftPos.z - handSize, rightPos.z - handSize});
		aabb.max.z = (std::max)({aabb.max.z, leftPos.z + handSize, rightPos.z + handSize});
	}

	return aabb;
}