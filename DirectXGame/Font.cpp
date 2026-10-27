#include "Font.h"
#include "LightManager.h"
#include "Matrix4x4.h"
#include <algorithm> // ★ 追加: std::min 用
#include <numbers>   // ★ 追加: π参照用
#include <string>

using namespace KamataEngine;

Font::~Font() {
	// 各モデルのメモリ解放
	delete modelCombo_;
	for (auto* model : modelNumbers_) {
		delete model;
	}

	// ★ 追加: 処刑演出用UIモデルのメモリ解放
	delete modelHitIt_;
	delete modelPushE_;

	// ★ 追加: ポーズ・ESC UIモデルのメモリ解放
	delete modelPause_;
	delete modelEsc_;

	// ワールドトランスフォームのメモリ解放
	for (auto* transform : worldTransformDigits_) {
		delete transform;
	}
	worldTransformDigits_.clear();
}

void Font::Initialize(Camera* camera) {
	camera_ = camera;

	// 1. "Combo" モデルの読み込み
	modelCombo_ = Model::CreateFromOBJ("Combo", true);

	// 2. "0" ～ "9" の数字モデルの読み込み
	for (int i = 0; i < 10; ++i) {
		std::string modelName = std::to_string(i);
		modelNumbers_[i] = Model::CreateFromOBJ(modelName, true);
	}

	// ★ 追加: 処刑演出用UIモデル（HitIt, PushE）の読み込み
	modelHitIt_ = Model::CreateFromOBJ("HitIt", true);
	modelPushE_ = Model::CreateFromOBJ("PushE", true);

	// ★ 追加: ポーズ・ESC UIモデルの読み込み
	modelPause_ = Model::CreateFromOBJ("Pause", true);
	modelEsc_ = Model::CreateFromOBJ("ESC", true);

	// ワールドトランスフォーム初期化
	worldTransformCombo_.Initialize();
	worldTransformHitIt_.Initialize();
	worldTransformPushE_.Initialize();
	worldTransformPause_.Initialize();
	worldTransformEsc_.Initialize();

	comboCount_ = 0;
	comboTimer_ = 0.0f;
	appearTimer_ = 0.0f;
	hitAnimTimer_ = 0.0f;

	// ★ 追加: 処刑演出UI初期化
	isExecutionUIVisible_ = false;
	executionUIAppearTimer_ = 0.0f;
	executionUIDisappearTimer_ = 0.0f;

	// ★ 追加: ポーズUIおよびEndScene遷移フラグ初期化
	isPauseUIVisible_ = true;
	isEndSceneRequested_ = false;
}

void Font::ApplyLightManager(const LightManager& lightManager) {
	// ★ 全フォントモデルにライティンググループ（ワールド用）を適用
	lightManager.ApplyToModel(modelCombo_, LightManager::GroupType::kWorld);
	for (auto* model : modelNumbers_) {
		if (model) {
			lightManager.ApplyToModel(model, LightManager::GroupType::kWorld);
		}
	}

	// ★ 追加: 処刑演出用UIモデルにライティンググループを適用
	if (modelHitIt_) {
		lightManager.ApplyToModel(modelHitIt_, LightManager::GroupType::kWorld);
	}
	if (modelPushE_) {
		lightManager.ApplyToModel(modelPushE_, LightManager::GroupType::kWorld);
	}

	// ★ 追加: ポーズ・ESC UIモデルにライティンググループを適用
	if (modelPause_) {
		lightManager.ApplyToModel(modelPause_, LightManager::GroupType::kWorld);
	}
	if (modelEsc_) {
		lightManager.ApplyToModel(modelEsc_, LightManager::GroupType::kWorld);
	}
}

void Font::AddCombo(int amount) {
	// ★ 非表示状態から新たにコンボが開始された場合、出現タイマーをリセット
	if (comboCount_ <= 0) {
		appearTimer_ = 0.0f;
	}

	comboCount_ += amount;
	comboTimer_ = kComboLimitTime; // タイマーをリセットして再スタート

	// ★ 毎回コンボ数が貯まる時のヒットアニメーションをスタート
	hitAnimTimer_ = kHitAnimDuration;
}

void Font::ResetCombo() {
	comboCount_ = 0;
	comboTimer_ = 0.0f;
	appearTimer_ = 0.0f;
	hitAnimTimer_ = 0.0f;
}

void Font::SetExecutionUIVisible(bool visible) {
	if (isExecutionUIVisible_ != visible) {
		isExecutionUIVisible_ = visible;
		if (isExecutionUIVisible_) {
			// 表示開始時は出現タイマーをリセット
			executionUIAppearTimer_ = 0.0f;
			executionUIDisappearTimer_ = kExecutionUIDisappearDuration;
		} else {
			// 非表示開始時は消滅タイマーを開始設定
			executionUIDisappearTimer_ = kExecutionUIDisappearDuration;
		}
	}
}

void Font::Update(bool isCameraPerformance) {
	float dt = 1.0f / 60.0f;

	// ==================== ポーズ UI (Pause, ESC) 更新 & 入力処理 ====================
	
	if (isPauseUIVisible_ && camera_) {
		// ★ 修正: Scaleを小さく変更 (1.0f -> 0.4f)
		Vector3 finalScale = {0.4f, 0.4f, 0.4f};
		Vector3 finalRot = {0.0f, std::numbers::pi_v<float>, 0.0f};

		// ★ PlayerHpの描画方法を参考に、カメラの座標からオフセットを加算してワールド座標を設定
		Vector3 basePosPause = {camera_->translation_.x + displayOffsetPause_.x, camera_->translation_.y + displayOffsetPause_.y, camera_->translation_.z + displayOffsetPause_.z};
		Vector3 basePosEsc = {camera_->translation_.x + displayOffsetEsc_.x, camera_->translation_.y + displayOffsetEsc_.y, camera_->translation_.z + displayOffsetEsc_.z};

		// "Pause" の位置設定
		worldTransformPause_.translation_ = basePosPause;
		worldTransformPause_.rotation_ = finalRot;
		worldTransformPause_.scale_ = finalScale;
		Matrix4x4 matPause = MakeAffineMatrix(worldTransformPause_.scale_, worldTransformPause_.rotation_, worldTransformPause_.translation_);
		worldTransformPause_.matWorld_ = matPause;
		worldTransformPause_.TransferMatrix();

		// "ESC" の位置設定
		worldTransformEsc_.translation_ = basePosEsc;
		worldTransformEsc_.rotation_ = finalRot;
		worldTransformEsc_.scale_ = finalScale;
		Matrix4x4 matEsc = MakeAffineMatrix(worldTransformEsc_.scale_, worldTransformEsc_.rotation_, worldTransformEsc_.translation_);
		worldTransformEsc_.matWorld_ = matEsc;
		worldTransformEsc_.TransferMatrix();
	}

	// ==================== コンボ表示更新 ====================
	if (comboCount_ > 0) {
		// ★ カメラ演出中でない場合のみコンボ消えタイマーを減算
		if (!isCameraPerformance) {
			comboTimer_ -= dt;
		}

		// 指定秒数間（kComboLimitTime）コンボが貯まらなかった場合、描画を消す
		if (comboTimer_ <= 0.0f) {
			ResetCombo();
		} else {
			// ★ 1. 出現時と消える時の演出（Z軸Rotation & XYZ Scale）の計算
			float appearScale = 1.0f;
			float appearRotZ = 0.0f;

			if (appearTimer_ < kAppearDuration) {
				// 出現アニメーション（拡大しながらポップアップ）
				appearTimer_ += dt;
				float progress = std::clamp(appearTimer_ / kAppearDuration, 0.0f, 1.0f);
				appearScale = std::sin(progress * std::numbers::pi_v<float> * 0.5f) * 1.2f - (progress * 0.2f); // 少しオーバーシュートして弾む
				appearRotZ = (1.0f - progress) * -0.3f;                                                         // Z軸に少し傾いた状態から戻る
			} else if (comboTimer_ < kDisappearDuration) {
				// 消滅アニメーション（縮小しながら回転して消える）
				float progress = std::clamp(comboTimer_ / kDisappearDuration, 0.0f, 1.0f);
				appearScale = progress;
				appearRotZ = (1.0f - progress) * 0.3f;
			}

			// ★ 2. 毎回コンボ数が貯まる時のヒットアニメーション（Z軸Rotation & XYZ Scale）の計算
			float hitScale = 1.0f;
			float hitRotZ = 0.0f;

			if (hitAnimTimer_ > 0.0f) {
				hitAnimTimer_ -= dt;
				float progress = 1.0f - (std::max)(hitAnimTimer_, 0.0f) / kHitAnimDuration;
				float sineVal = std::sin(progress * std::numbers::pi_v<float>);
				hitScale = 1.0f + sineVal * 0.35f; // 打撃の瞬間に拡大
				hitRotZ = sineVal * 0.15f;         // 打撃の瞬間にZ軸回転
			}

			// ★ アニメーション結果の合成
			float finalScaleFactor = appearScale * hitScale;
			Vector3 finalScale = {finalScaleFactor, finalScaleFactor, finalScaleFactor};
			Vector3 finalRot = {0.0f, std::numbers::pi_v<float>, appearRotZ + hitRotZ};

			// カメラ追従のトランスフォーム計算
			if (camera_) {
				// カメラの位置にオフセットを加算して画面上の定位置に表示
				Vector3 basePos;

				basePos.x = camera_->translation_.x + displayOffset_.x;
				basePos.y = camera_->translation_.y + displayOffset_.y;
				basePos.z = camera_->translation_.z + displayOffset_.z;

				// 1. Comboロゴの位置設定
				worldTransformCombo_.translation_ = basePos;
				worldTransformCombo_.rotation_ = finalRot;
				worldTransformCombo_.scale_ = finalScale;

				Matrix4x4 matWorld = MakeAffineMatrix(worldTransformCombo_.scale_, worldTransformCombo_.rotation_, worldTransformCombo_.translation_);
				worldTransformCombo_.matWorld_ = matWorld;
				worldTransformCombo_.TransferMatrix();

				// 2. 数字モデル（桁数分）の位置設定
				std::string comboStr = std::to_string(comboCount_);
				size_t numDigits = comboStr.length();

				if (worldTransformDigits_.size() < numDigits) {
					size_t oldSize = worldTransformDigits_.size();
					worldTransformDigits_.resize(numDigits);
					for (size_t i = oldSize; i < numDigits; ++i) {
						worldTransformDigits_[i] = new WorldTransform();
						worldTransformDigits_[i]->Initialize();
					}
				}

				// "Combo" ロゴの右側に各数字を並べる配置設定
				float digitSpacing = 0.45f;      // ★ 修正: 数字モデル同士のX間隔のみを狭く調整（10の位と1の位の間隔）
				float startX = basePos.x + 2.5f; // Comboロゴから最初の数字までの距離はそのまま維持

				for (size_t i = 0; i < numDigits; ++i) {
					Vector3 digitPos = basePos;
					digitPos.x = startX + static_cast<float>(i) * digitSpacing;

					worldTransformDigits_[i]->translation_ = digitPos;
					worldTransformDigits_[i]->rotation_ = finalRot;
					worldTransformDigits_[i]->scale_ = finalScale;

					Matrix4x4 matDigit = MakeAffineMatrix(worldTransformDigits_[i]->scale_, worldTransformDigits_[i]->rotation_, worldTransformDigits_[i]->translation_);
					worldTransformDigits_[i]->matWorld_ = matDigit;
					worldTransformDigits_[i]->TransferMatrix();
				}
			}
		}
	}

	// ==================== 処刑演出UI (HitIt, PushE) 更新 ====================
	if (isExecutionUIVisible_ || executionUIDisappearTimer_ > 0.0f) {
		float execScale = 1.0f;
		float execRotZ = 0.0f;

		if (isExecutionUIVisible_) {
			// ★ 出現アニメーション（拡大しながらポップアップ）
			if (executionUIAppearTimer_ < kExecutionUIAppearDuration) {
				executionUIAppearTimer_ += dt;
				float progress = std::clamp(executionUIAppearTimer_ / kExecutionUIAppearDuration, 0.0f, 1.0f);
				execScale = std::sin(progress * std::numbers::pi_v<float> * 0.5f) * 1.2f - (progress * 0.2f); // 少しオーバーシュートして弾む
				execRotZ = (1.0f - progress) * -0.3f;                                                         // Z軸に少し傾いた状態から戻る
			}
		} else {
			// ★ 消滅アニメーション（縮小しながら回転して消える）
			executionUIDisappearTimer_ -= dt;
			float progress = std::clamp(executionUIDisappearTimer_ / kExecutionUIDisappearDuration, 0.0f, 1.0f);
			execScale = progress;
			execRotZ = (1.0f - progress) * 0.3f;
		}

		if (camera_) {
			Vector3 finalScale = {execScale, execScale, execScale};
			Vector3 finalRot = {0.0f, std::numbers::pi_v<float>, execRotZ};

			// ★ カメラ追従相対座標によって処刑演出用UIモデル（HitIt, PushE）を配置
			Vector3 basePosHitIt = {camera_->translation_.x + displayOffsetHitIt_.x, camera_->translation_.y + displayOffsetHitIt_.y, camera_->translation_.z + displayOffsetHitIt_.z};
			Vector3 basePosPushE = {camera_->translation_.x + displayOffsetPushE_.x, camera_->translation_.y + displayOffsetPushE_.y, camera_->translation_.z + displayOffsetPushE_.z};

			// "HitIt" の位置設定
			worldTransformHitIt_.translation_ = basePosHitIt;
			worldTransformHitIt_.rotation_ = finalRot;
			worldTransformHitIt_.scale_ = finalScale;
			Matrix4x4 matHitIt = MakeAffineMatrix(worldTransformHitIt_.scale_, worldTransformHitIt_.rotation_, worldTransformHitIt_.translation_);
			worldTransformHitIt_.matWorld_ = matHitIt;
			worldTransformHitIt_.TransferMatrix();

			// "PushE" の位置設定
			worldTransformPushE_.translation_ = basePosPushE;
			worldTransformPushE_.rotation_ = finalRot;
			worldTransformPushE_.scale_ = finalScale;
			Matrix4x4 matPushE = MakeAffineMatrix(worldTransformPushE_.scale_, worldTransformPushE_.rotation_, worldTransformPushE_.translation_);
			worldTransformPushE_.matWorld_ = matPushE;
			worldTransformPushE_.TransferMatrix();
		}
	}
}

void Font::Draw() {
	// コンボ数が0以下の時は描画しない
	if (comboCount_ <= 0 || !camera_) {
		return;
	}

	// "Combo" モデルの描画
	if (modelCombo_) {
		modelCombo_->Draw(worldTransformCombo_, *camera_);
	}

	// コンボ数（数字モデル）の描画
	std::string comboStr = std::to_string(comboCount_);

	// 桁数が増えた直後でも配列外参照を起こさないよう、現在確保されている要素数以内に制限
	size_t drawCount = (std::min)(comboStr.length(), worldTransformDigits_.size());

	for (size_t i = 0; i < drawCount; ++i) {
		int digit = comboStr[i] - '0';
		if (digit >= 0 && digit <= 9 && modelNumbers_[digit] && worldTransformDigits_[i]) {
			modelNumbers_[digit]->Draw(*worldTransformDigits_[i], *camera_);
		}
	}
}

void Font::DrawExecutionUI() {
	// 処刑演出UIが非表示かつ消滅アニメーションも終了している場合は描画しない
	if ((!isExecutionUIVisible_ && executionUIDisappearTimer_ <= 0.0f) || !camera_) {
		return;
	}

	// ★ 修正: 中ボスの処刑演出用 UI モデル（HitIt, PushE）の描画
	if (modelHitIt_) {
		modelHitIt_->Draw(worldTransformHitIt_, *camera_);
	}
	if (modelPushE_) {
		modelPushE_->Draw(worldTransformPushE_, *camera_);
	}
}

void Font::DrawPauseUI() {
	if (!isPauseUIVisible_ || !camera_) {
		return;
	}

	// ★ PlayerHpの描画方法を参考に、描画直前にカメラの最新座標でワールド行列を再更新してカメラ追従時のブレを解消
	Vector3 finalScale = {0.4f, 0.4f, 0.4f};
	Vector3 finalRot = {0.0f, std::numbers::pi_v<float>, 0.0f};

	Vector3 basePosPause = {camera_->translation_.x + displayOffsetPause_.x, camera_->translation_.y + displayOffsetPause_.y, camera_->translation_.z + displayOffsetPause_.z};
	Vector3 basePosEsc = {camera_->translation_.x + displayOffsetEsc_.x, camera_->translation_.y + displayOffsetEsc_.y, camera_->translation_.z + displayOffsetEsc_.z};

	worldTransformPause_.translation_ = basePosPause;
	worldTransformPause_.rotation_ = finalRot;
	worldTransformPause_.scale_ = finalScale;
	worldTransformPause_.matWorld_ = MakeAffineMatrix(worldTransformPause_.scale_, worldTransformPause_.rotation_, worldTransformPause_.translation_);
	worldTransformPause_.TransferMatrix();

	worldTransformEsc_.translation_ = basePosEsc;
	worldTransformEsc_.rotation_ = finalRot;
	worldTransformEsc_.scale_ = finalScale;
	worldTransformEsc_.matWorld_ = MakeAffineMatrix(worldTransformEsc_.scale_, worldTransformEsc_.rotation_, worldTransformEsc_.translation_);
	worldTransformEsc_.TransferMatrix();

	// ★ 画面右上に Pause モデル、ちょっと下に ESC モデルを描画
	if (modelPause_) {
		modelPause_->Draw(worldTransformPause_, *camera_);
	}
	if (modelEsc_) {
		modelEsc_->Draw(worldTransformEsc_, *camera_);
	}
}