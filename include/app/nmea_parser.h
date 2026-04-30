#pragma once

#include <cstdint>

#include "app/gnss_state.h"

// 返回 true 表示 sentence 为受支持且字段校验通过的 GGA/RMC，并已更新 state。
// 返回 false 表示语句不受支持或字段非法，state 保持不变。
bool parseNmeaSentence(const char* sentence, std::uint32_t now_ms, GnssState& state);
