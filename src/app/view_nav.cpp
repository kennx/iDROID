#include "app/view_nav.h"

std::uint8_t moveNavSelection(std::uint8_t current, int delta, std::uint8_t item_count) {
  if (item_count <= 1) {
    return 0;
  }

  const int count = static_cast<int>(item_count);
  int next = static_cast<int>(current) + delta;
  while (next < 0) {
    next += count;
  }
  while (next >= count) {
    next -= count;
  }
  return static_cast<std::uint8_t>(next);
}

ViewId nextView(ViewId current, bool enter_pressed, bool go_pressed) {
  if (current == ViewId::kNav && enter_pressed) {
    return ViewId::kGnss;
  }
  if (current == ViewId::kGnss && go_pressed) {
    return ViewId::kNav;
  }
  return current;
}

ViewId nextViewFromNavSelection(std::uint8_t nav_index, bool enter_pressed) {
  if (!enter_pressed) {
    return ViewId::kNav;
  }

  if (nav_index == 1) {
    return ViewId::kBtKeyboard;
  }
  return ViewId::kGnss;
}
