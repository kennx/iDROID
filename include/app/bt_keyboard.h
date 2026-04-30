#pragma once

#include <cstdint>
#include <vector>

enum class BtKbdUiState : std::uint8_t {
  kIdle = 0,
  kAdvertising = 1,
  kConnected = 2,
  kDisconnected = 3,
  kInitFailed = 4,
};

struct BtKbdState {
  BtKbdUiState ui = BtKbdUiState::kIdle;
  bool service_started = false;
  bool connected = false;
};

enum class BtKbdSpecialKey : std::uint8_t {
  kEnter = 0,
  kBackspace = 1,
  kTab = 2,
  kSpace = 3,
};

BtKbdState enterBtKbd(bool init_ok);
BtKbdState onBtKbdConnected(BtKbdState state);
BtKbdState onBtKbdDisconnected(BtKbdState state);
BtKbdState exitBtKbd(BtKbdState state);
bool canSendBtKbdInput(const BtKbdState& state);

bool btKbdStartService(const char* device_name);
void btKbdStopService();
BtKbdState btKbdSyncConnection(BtKbdState state);
bool btKbdSendChar(char ch);
bool btKbdSendSpecial(BtKbdSpecialKey key);
bool btKbdSendHidStroke(std::uint8_t modifiers, const std::vector<std::uint8_t>& hid_keys);
