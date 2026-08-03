#include "BaseEnemyState.h"
#include <iostream>

void BaseEnemyState::DebugLog() {
	// スライド記載の Log(name_); 相当の出力処理
	std::cout << name_ << std::endl;
}