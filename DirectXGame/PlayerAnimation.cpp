#include "PlayerAnimation.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include "Matrix4x4.h"
#include <random>

// 行列の掛け算ヘルパー関数 (main.cppのコードを参考)
static KamataEngine::Matrix4x4 MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b) {
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

PlayerAnimation::PlayerAnimation() {}

PlayerAnimation::~PlayerAnimation() {
	// 個別モデルの削除
	delete modelPlayerHead_;
	delete modelPlayerBody_;
	delete modelPlayerLeft_;
	delete modelPlayerRight_;
}

void PlayerAnimation::Initialize() {
	// 4つの各部位のOBJファイルを読み取って適用
	modelPlayerHead_ = KamataEngine::Model::CreateFromOBJ("player_head", true);
	modelPlayerBody_ = KamataEngine::Model::CreateFromOBJ("player_body", true);
	modelPlayerLeft_ = KamataEngine::Model::CreateFromOBJ("player_left", true);
	modelPlayerRight_ = KamataEngine::Model::CreateFromOBJ("player_right", true);

	// 各部位のWorldTransform初期化
	worldTransformHead_.Initialize();
	worldTransformBody_.Initialize();
	worldTransformLeft_.Initialize();
	worldTransformRight_.Initialize();

	// 必要に応じて各部位の初期位置(ローカルのオフセット)を設定
	worldTransformBody_.translation_ = {0.0f, 0.0f, 0.0f};  // ルート（体）
	worldTransformHead_.translation_ = {0.0f, 0.0f, 0.0f};  // 体の上
	worldTransformLeft_.translation_ = {-0.0f, 0.0f, 0.0f}; // 体の左
	worldTransformRight_.translation_ = {0.0f, 0.0f, 0.0f}; // 体の右

	turnTimer_ = 0.0f;
	turnFirstRotationY_ = 0.0f;

	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0;

	walkAnimationTimer_ = 0.0f;

	// ハンマーのトランスフォーム初期化
	worldTransformHammer_.Initialize();
	hammerSkillTimer_ = 0.0f;
	isHammerVisible_ = false;
}

void PlayerAnimation::BehaviorRootInit() {
	// 通常状態に戻る際、スケールとZ軸回転(傾斜)をリセット
	worldTransformLeft_.rotation_.z = 0.0f;
	worldTransformRight_.rotation_.z = 0.0f;
	walkAnimationTimer_ = 0.0f;

	isHammerVisible_ = false;
}

void PlayerAnimation::BehaviorRootUpdate(KamataEngine::WorldTransform& worldTransform, LRDirection lrDirection, const KamataEngine::Vector3& velocity, bool onGround, float limitRunSpeed, float timeTurn) {
	if (turnTimer_ < timeTurn) {
		turnTimer_ += 1.0f / 60.0f;
		if (turnTimer_ > timeTurn) {
			turnTimer_ = timeTurn;
		}
	}

	const float pi = std::numbers::pi_v<float>;

	// 目標角度の設定
	// 右向き (kRight) : +PI / 2  (+90度)
	// 左向き (kLeft)  : -PI / 2  (-90度)
	float targetRotationY = (lrDirection == LRDirection::kRight) ? (pi / 2.0f) : (-pi / 2.0f);

	// 開始角度から目標角度への最短の差分(diff)を求める
	float diff = targetRotationY - turnFirstRotationY_;

	// 差分を -PI ~ +PI の範囲に正規化する (常に180度以内の最短角度で回す)
	while (diff > pi) {
		diff -= 2.0f * pi;
	}
	while (diff < -pi) {
		diff += 2.0f * pi;
	}

	// イージング(EaseOut)を適用して回転
	float ratio = turnTimer_ / timeTurn;
	float easeRatio = EaseOut(0.0f, 1.0f, ratio);

	worldTransform.rotation_.y = turnFirstRotationY_ + diff * easeRatio;

	// 歩きアニメーション処理を追加
	if (onGround && std::abs(velocity.x) > 0.01f) {
		// 移動速度に応じてアニメーション時間を進める
		walkAnimationTimer_ += (std::abs(velocity.x) / limitRunSpeed) * 0.025f;

		// Z軸の角度を使って左右のパーツを交互にスイングさせる（最大約25度傾く想定）
		float walkTilt = std::sin(walkAnimationTimer_ * 2.0f * pi) * 0.75f;
		worldTransformLeft_.rotation_.x = walkTilt;
		worldTransformRight_.rotation_.x = -walkTilt;
	} else {
		// 移動していないか空中にいる時は徐々に直立に戻す
		worldTransformLeft_.rotation_.x *= 0.8f;
		worldTransformRight_.rotation_.x *= 0.8f;
		if (std::abs(worldTransformLeft_.rotation_.x) < 0.001f) {
			worldTransformLeft_.rotation_.x = 0.0f;
			worldTransformRight_.rotation_.x = 0.0f;
			walkAnimationTimer_ = 0.0f;
		}
	}

	// プレイヤー全体のルート行列を計算
	worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	worldTransform.TransferMatrix();

	// 各ノードのローカル行列（LocalMatrix）を計算
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);

	// 中心を下にずらす ＝ モデルの見た目を相対的に上に持っていくため、Y軸にプラスします
	KamataEngine::Vector3 centerOffset = {0.0f, -0.5f, 0.0f};

	// 【Left Arm/Leg】
	// ① 原点中心の回転行列
	KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformLeft_.rotation_, {0.0f, 0.0f, 0.0f});
	// ② 中心をずらすための逆移動行列（前後の移動で回転中心をオフセット）
	KamataEngine::Matrix4x4 preTranslateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});

	// ③ 本来の座標（translation_）に中心のオフセットを足した位置へ移動する行列
	KamataEngine::Vector3 finalTranslationLeft = {
	    worldTransformLeft_.translation_.x + centerOffset.x, worldTransformLeft_.translation_.y + centerOffset.y, worldTransformLeft_.translation_.z + centerOffset.z};
	KamataEngine::Matrix4x4 postTranslateLeft = MakeAffineMatrix(worldTransformLeft_.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationLeft);

	// ④ すべてを合成 (右から順に適用: 中心移動 ➔ 回転 ➔ 本来の位置へ配置)
	KamataEngine::Matrix4x4 localMatrixLeft = MultiplyMatrix(postTranslateLeft, MultiplyMatrix(rotateLeft, preTranslateLeft));

	// 【Right Arm/Leg】
	// ① 原点中心の回転行列
	KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformRight_.rotation_, {0.0f, 0.0f, 0.0f});
	// ② 中心をずらすための逆移動行列
	KamataEngine::Matrix4x4 preTranslateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});

	// ③ 本来の座標に中心のオフセットを足した位置へ移動する行列
	KamataEngine::Vector3 finalTranslationRight = {
	    worldTransformRight_.translation_.x + centerOffset.x, worldTransformRight_.translation_.y + centerOffset.y, worldTransformRight_.translation_.z + centerOffset.z};
	KamataEngine::Matrix4x4 postTranslateRight = MakeAffineMatrix(worldTransformRight_.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationRight);

	// ④ すべてを合成
	KamataEngine::Matrix4x4 localMatrixRight = MultiplyMatrix(postTranslateRight, MultiplyMatrix(rotateRight, preTranslateRight));

	// 親子関係に基づきワールド行列（WorldMatrix）を計算
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	// 各部位の行列を転送
	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();
}

void PlayerAnimation::BehaviorAttackInit() {
	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0;
	// 攻撃移行時は歩き用のZ軸回転角度をリセット
	worldTransformLeft_.rotation_.z = 0.0f;
	worldTransformRight_.rotation_.z = 0.0f;
}

void PlayerAnimation::BehaviorAttackUpdate(KamataEngine::WorldTransform& worldTransform, LRDirection lrDirection, float attackVelocity, KamataEngine::Vector3& outVelocity, bool& outCreateHitEffect, bool& outFinished) {
	attackParameter_++;
	outVelocity = {};
	outCreateHitEffect = false;
	outFinished = false;

	// 傾き角度の設定
	const float kMaxTiltAngle = 0.4f; // 前傾の最大角度（ラジアン）

	// 【修正】符号を反転
	// 右向き：プラス回転(Z軸)で前傾、左向き：マイナス回転(Z軸)で前傾
	float tiltSign = (lrDirection == LRDirection::kRight) ? 1.0f : -1.0f;
	float targetTilt = kMaxTiltAngle * tiltSign;

	// スケールは固定
	worldTransform.scale_ = {1.0f, 1.0f, 1.0f};

	switch (attackPhase_) {
	case AttackPhase::kCharge:
	default: {
		float t = static_cast<float>(attackParameter_) / static_cast<float>(kChargeDuration);
		t = std::min<float>(t, 1.0f);

		// 0から前傾角度まで滑らかに傾斜
		worldTransform.rotation_.z = EaseOut(0.0f, targetTilt, t);

		if (attackParameter_ >= kChargeDuration) {
			attackPhase_ = AttackPhase::kDash;
			attackParameter_ = 0;

			outCreateHitEffect = true;
		}
		break;
	}

	case AttackPhase::kDash: {
		// 突進中は傾きを保持
		worldTransform.rotation_.z = targetTilt;

		if (lrDirection == LRDirection::kRight) {
			outVelocity.x = +attackVelocity;
		} else {
			outVelocity.x = -attackVelocity;
		}

		if (attackParameter_ >= kDashDuration) {
			attackPhase_ = AttackPhase::kRecoil;
			attackParameter_ = 0;
		}
		break;
	}

	case AttackPhase::kRecoil: {
		float t = static_cast<float>(attackParameter_) / static_cast<float>(kRecoilDuration);
		t = std::min<float>(t, 1.0f);

		// 前傾姿勢から元の直立(0)へ滑らかに戻る
		worldTransform.rotation_.z = EaseOut(targetTilt, 0.0f, t);

		if (attackParameter_ >= kRecoilDuration) {
			worldTransform.rotation_.z = 0.0f;
			outFinished = true;
		}
		break;
	}
	}

	worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	worldTransform.TransferMatrix();

	// 攻撃時も同様に各メッシュの親子関係行列を計算
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);

	// ★【修正】攻撃時も回転中心の修正を適用
	KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformLeft_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 translateLeft = MakeAffineMatrix(worldTransformLeft_.scale_, {0.0f, 0.0f, 0.0f}, worldTransformLeft_.translation_);
	KamataEngine::Matrix4x4 localMatrixLeft = MultiplyMatrix(translateLeft, rotateLeft);

	KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformRight_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 translateRight = MakeAffineMatrix(worldTransformRight_.scale_, {0.0f, 0.0f, 0.0f}, worldTransformRight_.translation_);
	KamataEngine::Matrix4x4 localMatrixRight = MultiplyMatrix(translateRight, rotateRight);

	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();
}

void PlayerAnimation::BehaviorKnockbackInit() {
	// ノックバック移行時も回転角度をクリア
	worldTransformLeft_.rotation_.z = 0.0f;
	worldTransformRight_.rotation_.z = 0.0f;
}

void PlayerAnimation::BehaviorKnockbackUpdate(KamataEngine::WorldTransform& worldTransform) {
	// 以下、既存の行列計算と転送処理
	worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	worldTransform.TransferMatrix();

	// ノックバック時も親子関係行列を更新
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);

	// ★ノックバック時も回転中心の修正を適用
	KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformLeft_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 translateLeft = MakeAffineMatrix(worldTransformLeft_.scale_, {0.0f, 0.0f, 0.0f}, worldTransformLeft_.translation_);
	KamataEngine::Matrix4x4 localMatrixLeft = MultiplyMatrix(translateLeft, rotateLeft);

	KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformRight_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 translateRight = MakeAffineMatrix(worldTransformRight_.scale_, {0.0f, 0.0f, 0.0f}, worldTransformRight_.translation_);
	KamataEngine::Matrix4x4 localMatrixRight = MultiplyMatrix(translateRight, rotateRight);

	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();
}

void PlayerAnimation::BehaviorHammerSkillInit() {
	hammerSkillTimer_ = 0.0f;
	isHammerVisible_ = false;
}

void PlayerAnimation::BehaviorHammerSkillUpdate(KamataEngine::WorldTransform& worldTransform, bool& outFinished) {
	hammerSkillTimer_ += 1.0f / 60.0f;
	outFinished = false;

	float progress = hammerSkillTimer_ / kHammerSkillDuration;
	if (progress > 1.0f) {
		progress = 1.0f;
	}

	float armRotationX = 0.0f;

	// アニメーションの前半（手を上げる）と後半（振り下ろす）
	if (progress < 0.4f) {
		// 0.0 ~ 0.4 の間で両手を上に上げる
		float t = progress / 0.4f;
		armRotationX = EaseOut(0.0f, -4.0f, t);
		isHammerVisible_ = true; // 手を上げ始めると同時にハンマーを表示
	} else {
		// 0.4 ~ 1.0 の間で一気に振り下ろす
		float t = (progress - 0.4f) / 0.6f;
		armRotationX = EaseOut(-4.0f, -1.5f, t);
	}

	// 両手に同じ回転を適用（左右対称に上げる）
	worldTransformLeft_.rotation_.x = armRotationX;
	worldTransformRight_.rotation_.x = armRotationX;

	// プレイヤー本体のベース行列の計算
	worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	worldTransform.TransferMatrix();

	// 各部位のローカル・ワールド行列計算
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform.matWorld_);
	worldTransformBody_.TransferMatrix();

	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformHead_.TransferMatrix();

	// 回転の中心オフセットを考慮した腕の行列計算
	KamataEngine::Vector3 centerOffset = {0.0f, -0.5f, 0.0f};

	// 左手
	KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformLeft_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 preTranslateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
	KamataEngine::Vector3 finalTranslationLeft = {
	    worldTransformLeft_.translation_.x + centerOffset.x, worldTransformLeft_.translation_.y + centerOffset.y, worldTransformLeft_.translation_.z + centerOffset.z};
	KamataEngine::Matrix4x4 postTranslateLeft = MakeAffineMatrix(worldTransformLeft_.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationLeft);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(postTranslateLeft, MultiplyMatrix(rotateLeft, preTranslateLeft));
	worldTransformLeft_.matWorld_ = MultiplyMatrix(worldTransformLeft_.matWorld_, worldTransformBody_.matWorld_);
	worldTransformLeft_.TransferMatrix();

	// 右手
	KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, worldTransformRight_.rotation_, {0.0f, 0.0f, 0.0f});
	KamataEngine::Matrix4x4 preTranslateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
	KamataEngine::Vector3 finalTranslationRight = {
	    worldTransformRight_.translation_.x + centerOffset.x, worldTransformRight_.translation_.y + centerOffset.y, worldTransformRight_.translation_.z + centerOffset.z};
	KamataEngine::Matrix4x4 postTranslateRight = MakeAffineMatrix(worldTransformRight_.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationRight);
	worldTransformRight_.matWorld_ = MultiplyMatrix(postTranslateRight, MultiplyMatrix(rotateRight, preTranslateRight));
	worldTransformRight_.matWorld_ = MultiplyMatrix(worldTransformRight_.matWorld_, worldTransformBody_.matWorld_);
	worldTransformRight_.TransferMatrix();

	// ★ ハンマーを右手に追従させる制御
	if (isHammerVisible_) {
		KamataEngine::Vector3 hammerLocalPos = {0.0f, 0.5f, 0.2f};
		KamataEngine::Vector3 hammerLocalRot = {-3.0f, 0.0f, 0.0f};

		KamataEngine::Matrix4x4 localMatrixHammer = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, hammerLocalRot, hammerLocalPos);
		worldTransformHammer_.matWorld_ = MultiplyMatrix(localMatrixHammer, worldTransformRight_.matWorld_);
		worldTransformHammer_.TransferMatrix();
	}

	// アニメーション終了判定
	if (hammerSkillTimer_ >= kHammerSkillDuration) {
		outFinished = true;
		isHammerVisible_ = false;
	}
}

void PlayerAnimation::Draw(KamataEngine::Camera* camera, KamataEngine::Model* modelHammer) {
	if (camera) {
		if (modelPlayerBody_ && modelPlayerHead_ && modelPlayerLeft_ && modelPlayerRight_) {
			modelPlayerBody_->Draw(worldTransformBody_, *camera);
			modelPlayerHead_->Draw(worldTransformHead_, *camera);
			modelPlayerLeft_->Draw(worldTransformLeft_, *camera);
			modelPlayerRight_->Draw(worldTransformRight_, *camera);

			// ★ ハンマーの描画処理を追加（スキル発動中かつモデルが存在する場合）
			if (isHammerVisible_ && modelHammer) {
				modelHammer->Draw(worldTransformHammer_, *camera);
			}
		}
	}
}

void PlayerAnimation::BehaviorDeathInit(const KamataEngine::WorldTransform& worldTransform) {
	deathTimer_ = 0.0f;
	isHammerVisible_ = false; // 死亡時はハンマーを非表示

	// 乱数生成（部位ごとに上方向＋ランダムな方向へ跳ね飛ばす）
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distSpeedX(-0.15f, 0.15f);
	std::uniform_real_distribution<float> distSpeedY(0.2f, 0.35f); // 上向きの初期速
	std::uniform_real_distribution<float> distSpeedZ(-0.1f, 0.1f);
	std::uniform_real_distribution<float> distRot(-0.2f, 0.2f);

	for (int i = 0; i < 4; ++i) {
		deathPieceVelocities_[i].velocity = {distSpeedX(gen), distSpeedY(gen), distSpeedZ(gen)};
		deathPieceVelocities_[i].rotationVelocity = {distRot(gen), distRot(gen), distRot(gen)};
	}

	// 各部位の初期ワールド位置を設定（プレイヤー中心からのオフセット）
	const KamataEngine::Vector3& basePos = worldTransform.translation_;
	worldTransformHead_.translation_ = {basePos.x, basePos.y + 0.5f, basePos.z};
	worldTransformBody_.translation_ = basePos;
	worldTransformLeft_.translation_ = {basePos.x - 0.3f, basePos.y, basePos.z};
	worldTransformRight_.translation_ = {basePos.x + 0.3f, basePos.y, basePos.z};

	// 回転角度のリセット
	worldTransformHead_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldTransformBody_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldTransformLeft_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldTransformRight_.rotation_ = {0.0f, 0.0f, 0.0f};
}

void PlayerAnimation::BehaviorDeathUpdate(bool& outFinished) {
	deathTimer_ += 1.0f / 60.0f;
	outFinished = false;

	const float kGravity = 0.012f; // 放物線を描かせるための重力加速度

	KamataEngine::WorldTransform* transforms[4] = {&worldTransformHead_, &worldTransformBody_, &worldTransformLeft_, &worldTransformRight_};

	for (int i = 0; i < 4; ++i) {
		// 移動更新と重力適用
		transforms[i]->translation_.x += deathPieceVelocities_[i].velocity.x;
		transforms[i]->translation_.y += deathPieceVelocities_[i].velocity.y;
		transforms[i]->translation_.z += deathPieceVelocities_[i].velocity.z;
		deathPieceVelocities_[i].velocity.y -= kGravity;

		// 回転更新
		transforms[i]->rotation_.x += deathPieceVelocities_[i].rotationVelocity.x;
		transforms[i]->rotation_.y += deathPieceVelocities_[i].rotationVelocity.y;
		transforms[i]->rotation_.z += deathPieceVelocities_[i].rotationVelocity.z;

		// 独立した部位のワールド行列計算・転送
		transforms[i]->matWorld_ = MakeAffineMatrix(transforms[i]->scale_, transforms[i]->rotation_, transforms[i]->translation_);
		transforms[i]->TransferMatrix();
	}

	if (deathTimer_ >= kDeathDuration) {
		outFinished = true;
	}
}