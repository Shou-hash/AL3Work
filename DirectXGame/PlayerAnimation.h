#pragma once
#include "KamataEngine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <vector>

enum class LRDirection;

enum class AttackPhase {
	kCharge,
	kDash,
	kRecoil,
};

class PlayerAnimation {
public:
	PlayerAnimation();
	~PlayerAnimation();

	void Initialize();

	void BehaviorRootInit();
	void BehaviorAttackInit();
	void BehaviorKnockbackInit();

	void BehaviorHammerSkillInit(uint32_t comboIndex = 0);                      // ★コンボ段階を受け取れるように拡張

	void BehaviorDeathInit(const KamataEngine::WorldTransform& worldTransform); // ★追加

	void BehaviorRootUpdate(KamataEngine::WorldTransform& worldTransform, LRDirection lrDirection, const KamataEngine::Vector3& velocity, bool onGround, float limitRunSpeed, float timeTurn);
	void BehaviorAttackUpdate(
	    KamataEngine::WorldTransform& worldTransform, LRDirection lrDirection, float attackVelocity, KamataEngine::Vector3& outVelocity, bool& outCreateHitEffect, bool& outFinished);
	void BehaviorKnockbackUpdate(KamataEngine::WorldTransform& worldTransform);
	
	// ★ハンマー連撃用アップデート関数（Hit判定・コンボ受付・終了フラグを返却）
	void BehaviorHammerSkillUpdate(KamataEngine::WorldTransform& worldTransform, LRDirection lrDirection, bool& outHitCheck, bool& outCanCombo, bool& outFinished);
	
	void BehaviorDeathUpdate(bool& outFinished); // ★追加

	void Draw(KamataEngine::Camera* camera, KamataEngine::Model* modelHammer);

	// 各部位の個別WorldTransformを取得する関数
	KamataEngine::WorldTransform& GetWorldTransformHead() { return worldTransformHead_; }
	KamataEngine::WorldTransform& GetWorldTransformBody() { return worldTransformBody_; }
	KamataEngine::WorldTransform& GetWorldTransformLeft() { return worldTransformLeft_; }
	KamataEngine::WorldTransform& GetWorldTransformRight() { return worldTransformRight_; }
	KamataEngine::WorldTransform& GetWorldTransformHammer() { return worldTransformHammer_; }

	AttackPhase GetAttackPhase() const { return attackPhase_; }
	float GetHammerSkillTimer() const { return hammerSkillTimer_; }
	bool IsHammerVisible() const { return isHammerVisible_; }
	uint32_t GetComboIndex() const { return comboIndex_; } // ★現在のコンボ段数を取得
	static float GetHammerSkillDuration() { return kHammerSkillDurationStep0; }

	void OnDirectionChanged(float currentRotationY) {
		turnFirstRotationY_ = currentRotationY;
		turnTimer_ = 0.0f;
	}

	float GetCurrentHammerSkillDuration() const {
		switch (comboIndex_) {
		case 0:
			return kHammerSkillDurationStep0;
		case 1:
			return kHammerSkillDurationStep1;
		case 2:
			return kHammerSkillDurationStep2;
		case 3:
			return kHammerSkillDurationStep3;
		case 4:
			return kHammerSkillDurationStep4;
		default:
			return kHammerSkillDurationStep0;
		}
	}

private:
	float EaseOut(float start, float end, float t) {
		float easing = 1.0f - std::pow(1.0f - t, 3.0f);
		return start + (end - start) * easing;
	}

	float EaseIn(float start, float end, float t) {
		float easing = std::pow(t, 3.0f);
		return start + (end - start) * easing;
	}

private:
	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	// 各部位の個別モデルポインタ
	KamataEngine::Model* modelPlayerHead_ = nullptr;
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerLeft_ = nullptr;
	KamataEngine::Model* modelPlayerRight_ = nullptr;

	// 各部位の個別WorldTransform (0: Head, 1: Body, 2: Left, 3: Right)
	KamataEngine::WorldTransform worldTransformHead_;
	KamataEngine::WorldTransform worldTransformBody_;
	KamataEngine::WorldTransform worldTransformLeft_;
	KamataEngine::WorldTransform worldTransformRight_;

	// 歩きアニメーション用のタイマー変数
	float walkAnimationTimer_ = 0.0f;

	KamataEngine::WorldTransform worldTransformHammer_;

	// アニメーション制御用タイマーとフラグ
	AttackPhase attackPhase_ = AttackPhase::kCharge;
	uint32_t attackParameter_ = 0;

	static inline uint32_t kChargeDuration = 8;
	static inline uint32_t kDashDuration = 8;
	static inline uint32_t kRecoilDuration = 10;

	// ★ ハンマー連撃用変数
	uint32_t comboIndex_ = 0; // 0: 1段目(斜め切り), 1: 2段目(横払い), 2: 3段目(叩きつけ)
	float hammerSkillTimer_ = 0.0f;
	bool isHammerVisible_ = false;
	//static inline float kHammerSkillDuration = 0.6f;

	// 各コンボ段階ごとのアニメーション再生時間（秒）
	static inline float kHammerSkillDurationStep0 = 0.35f;
	static inline float kHammerSkillDurationStep1 = 0.35f;
	static inline float kHammerSkillDurationStep2 = 0.35f; // ★ 3段目用
	static inline float kHammerSkillDurationStep3 = 0.40f; // ★ 4段目用
	static inline float kHammerSkillDurationStep4 = 0.55f; // ★ 5段目(フィニッシュ)用

	// ★ 死亡飛散演出用の構造体と変数
	struct PieceVelocity {
		KamataEngine::Vector3 velocity;
		KamataEngine::Vector3 rotationVelocity;
	};
	std::array<PieceVelocity, 4> deathPieceVelocities_;
	float deathTimer_ = 0.0f;
	static inline float kDeathDuration = 1.5f; // アニメーション時間（1.5秒）
};