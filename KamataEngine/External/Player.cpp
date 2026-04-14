#include "Player.h"
#include <assert.h>

void Player::Initialize(KamataEngine::Model* model, uint32_t textHandle, KamataEngine::Camera* camera) {
	
	if (model == nullptr) 
	{
		assert(model != nullptr);
	}
	if (camera == nullptr) 
	{
		assert(camera != nullptr);
	}

	mModel = model;
	mTextHandle = textHandle;
	mCamera = camera;
	mWorldTransform.Initialize();
}

void Player::Update() 
{
	mWorldTransform.TransferMatrix(); 
}

void Player::Draw() 
{
	mModel->Draw(mWorldTransform, *mCamera, mTextHandle); 
}