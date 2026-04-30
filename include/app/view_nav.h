#pragma once

#include <cstdint>

enum class ViewId : std::uint8_t {
  kNav = 0,
  kGnss = 1,
  kBtKeyboard = 2,
};

std::uint8_t moveNavSelection(std::uint8_t current, int delta, std::uint8_t item_count);
ViewId nextView(ViewId current, bool enter_pressed, bool go_pressed);
ViewId nextViewFromNavSelection(std::uint8_t nav_index, bool enter_pressed);
