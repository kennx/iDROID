#include <unity.h>

#include "app/bt_keyboard.h"

void test_initial_state_is_idle() {
  BtKbdState s{};
  TEST_ASSERT_EQUAL_INT(static_cast<int>(BtKbdUiState::kIdle), static_cast<int>(s.ui));
  TEST_ASSERT_FALSE(s.service_started);
  TEST_ASSERT_FALSE(s.connected);
}

void test_enter_view_moves_to_advertising_on_success() {
  BtKbdState s = enterBtKbd(true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(BtKbdUiState::kAdvertising), static_cast<int>(s.ui));
  TEST_ASSERT_TRUE(s.service_started);
  TEST_ASSERT_FALSE(s.connected);
}

void test_enter_view_moves_to_init_failed_on_init_error() {
  BtKbdState s = enterBtKbd(false);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(BtKbdUiState::kInitFailed), static_cast<int>(s.ui));
  TEST_ASSERT_FALSE(s.service_started);
  TEST_ASSERT_FALSE(s.connected);
}

void test_disconnect_transitions_back_to_advertising() {
  BtKbdState s = enterBtKbd(true);
  s = onBtKbdConnected(s);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(BtKbdUiState::kConnected), static_cast<int>(s.ui));
  TEST_ASSERT_TRUE(s.connected);

  s = onBtKbdDisconnected(s);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(BtKbdUiState::kAdvertising), static_cast<int>(s.ui));
  TEST_ASSERT_FALSE(s.connected);
}

void test_exit_view_returns_idle_and_stops_service() {
  BtKbdState s = enterBtKbd(true);
  s = onBtKbdConnected(s);

  s = exitBtKbd(s);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(BtKbdUiState::kIdle), static_cast<int>(s.ui));
  TEST_ASSERT_FALSE(s.service_started);
  TEST_ASSERT_FALSE(s.connected);
}

void test_send_gate_depends_on_connected_state() {
  BtKbdState s = enterBtKbd(true);
  TEST_ASSERT_FALSE(canSendBtKbdInput(s));

  s = onBtKbdConnected(s);
  TEST_ASSERT_TRUE(canSendBtKbdInput(s));

  s = onBtKbdDisconnected(s);
  TEST_ASSERT_FALSE(canSendBtKbdInput(s));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state_is_idle);
  RUN_TEST(test_enter_view_moves_to_advertising_on_success);
  RUN_TEST(test_enter_view_moves_to_init_failed_on_init_error);
  RUN_TEST(test_disconnect_transitions_back_to_advertising);
  RUN_TEST(test_exit_view_returns_idle_and_stops_service);
  RUN_TEST(test_send_gate_depends_on_connected_state);
  return UNITY_END();
}
