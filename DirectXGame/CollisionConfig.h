#pragma once

#include <cstdint>

// プレイヤー陣営（自キャラ・自弾）
const uint32_t kCollisionAttributePlayer = 0b1;
// 敵陣営（敵キャラ・敵弾）
const uint32_t kCollisionAttributeEnemy = 0b1 << 1;
// アイテム
const uint32_t kCollisionAttributeItem = 0b1 << 2;