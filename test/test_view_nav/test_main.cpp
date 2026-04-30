#include <unity.h>

#include "app/view_nav.h"

void test_nav_move_keeps_index_when_single_item() {
  TEST_ASSERT_EQUAL_UINT8(0, moveNavSelection(0, +1, 1));
  TEST_ASSERT_EQUAL_UINT8(0, moveNavSelection(0, -1, 1));
}

void test_nav_move_wraps_with_multiple_items() {
  TEST_ASSERT_EQUAL_UINT8(2, moveNavSelection(0, -1, 3));
  TEST_ASSERT_EQUAL_UINT8(0, moveNavSelection(2, +1, 3));
}

void test_nav_move_supports_two_items() {
  TEST_ASSERT_EQUAL_UINT8(1, moveNavSelection(0, +1, 2));
  TEST_ASSERT_EQUAL_UINT8(0, moveNavSelection(1, +1, 2));
  TEST_ASSERT_EQUAL_UINT8(1, moveNavSelection(0, -1, 2));
}

void test_view_transition_from_nav_by_enter() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kGnss),
                        static_cast<int>(nextView(ViewId::kNav, true, false)));
}

void test_view_transition_from_gnss_by_go() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kNav),
                        static_cast<int>(nextView(ViewId::kGnss, false, true)));
}

void test_view_transition_ignores_irrelevant_keys() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kNav),
                        static_cast<int>(nextView(ViewId::kNav, false, true)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kGnss),
                        static_cast<int>(nextView(ViewId::kGnss, true, false)));
}

void test_next_view_from_nav_selection_bt_keyboard_with_enter() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kBtKeyboard),
                        static_cast<int>(nextViewFromNavSelection(1, true)));
}

void test_next_view_from_nav_selection_gnss_with_enter() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kGnss),
                        static_cast<int>(nextViewFromNavSelection(0, true)));
}

void test_next_view_from_nav_selection_without_enter_stays_nav() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kNav),
                        static_cast<int>(nextViewFromNavSelection(1, false)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nav_move_keeps_index_when_single_item);
  RUN_TEST(test_nav_move_wraps_with_multiple_items);
  RUN_TEST(test_nav_move_supports_two_items);
  RUN_TEST(test_view_transition_from_nav_by_enter);
  RUN_TEST(test_view_transition_from_gnss_by_go);
  RUN_TEST(test_view_transition_ignores_irrelevant_keys);
  RUN_TEST(test_next_view_from_nav_selection_bt_keyboard_with_enter);
  RUN_TEST(test_next_view_from_nav_selection_gnss_with_enter);
  RUN_TEST(test_next_view_from_nav_selection_without_enter_stays_nav);
  return UNITY_END();
}
