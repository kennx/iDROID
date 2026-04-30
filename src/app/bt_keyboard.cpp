#include "app/bt_keyboard.h"

#if defined(ARDUINO)
#include <NimBleKeyboard.h>
#endif

namespace {
#if defined(ARDUINO)
BleKeyboard* g_ble_keyboard = nullptr;
bool g_ble_started = false;

constexpr const char* kDefaultDeviceName = "iDROID-KBD";
constexpr const char* kManufacturer = "iDROID";
#endif
}  // namespace

BtKbdState enterBtKbd(bool init_ok) {
  BtKbdState state{};
  if (!init_ok) {
    state.ui = BtKbdUiState::kInitFailed;
    state.service_started = false;
    state.connected = false;
    return state;
  }

  state.ui = BtKbdUiState::kAdvertising;
  state.service_started = true;
  state.connected = false;
  return state;
}

BtKbdState onBtKbdConnected(BtKbdState state) {
  if (!state.service_started) {
    return state;
  }
  state.ui = BtKbdUiState::kConnected;
  state.connected = true;
  return state;
}

BtKbdState onBtKbdDisconnected(BtKbdState state) {
  if (!state.service_started) {
    return state;
  }
  state.ui = BtKbdUiState::kAdvertising;
  state.connected = false;
  return state;
}

BtKbdState exitBtKbd(BtKbdState state) {
  state.ui = BtKbdUiState::kIdle;
  state.service_started = false;
  state.connected = false;
  return state;
}

bool canSendBtKbdInput(const BtKbdState& state) {
  return state.service_started && state.connected && state.ui == BtKbdUiState::kConnected;
}

bool btKbdStartService(const char* device_name) {
#if !defined(ARDUINO)
  (void)device_name;
  return false;
#else
  if (g_ble_started && g_ble_keyboard != nullptr) {
    return true;
  }

  const char* name = device_name;
  if (name == nullptr || name[0] == '\0') {
    name = kDefaultDeviceName;
  }

  g_ble_keyboard = new BleKeyboard(name, kManufacturer, 100);
  if (g_ble_keyboard == nullptr) {
    return false;
  }

  g_ble_keyboard->begin();
  g_ble_started = true;
  return true;
#endif
}

void btKbdStopService() {
#if defined(ARDUINO)
  if (g_ble_keyboard != nullptr) {
    g_ble_keyboard->end();
    delete g_ble_keyboard;
    g_ble_keyboard = nullptr;
  }
  g_ble_started = false;
#endif
}

BtKbdState btKbdSyncConnection(BtKbdState state) {
#if !defined(ARDUINO)
  return state;
#else
  if (!state.service_started || g_ble_keyboard == nullptr) {
    return state;
  }

  if (g_ble_keyboard->isConnected()) {
    return onBtKbdConnected(state);
  }
  return onBtKbdDisconnected(state);
#endif
}

bool btKbdSendChar(char ch) {
#if !defined(ARDUINO)
  (void)ch;
  return false;
#else
  if (g_ble_keyboard == nullptr || !g_ble_keyboard->isConnected()) {
    return false;
  }
  return g_ble_keyboard->write(static_cast<std::uint8_t>(ch)) == 1;
#endif
}

bool btKbdSendSpecial(BtKbdSpecialKey key) {
#if !defined(ARDUINO)
  (void)key;
  return false;
#else
  if (g_ble_keyboard == nullptr || !g_ble_keyboard->isConnected()) {
    return false;
  }

  switch (key) {
    case BtKbdSpecialKey::kEnter:
      return g_ble_keyboard->write(KEY_RETURN) == 1;
    case BtKbdSpecialKey::kBackspace:
      return g_ble_keyboard->write(KEY_BACKSPACE) == 1;
    case BtKbdSpecialKey::kTab:
      return g_ble_keyboard->write(KEY_TAB) == 1;
    case BtKbdSpecialKey::kSpace:
      return g_ble_keyboard->write(static_cast<std::uint8_t>(' ')) == 1;
  }
  return false;
#endif
}

bool btKbdSendHidStroke(std::uint8_t modifiers, const std::vector<std::uint8_t>& hid_keys) {
#if !defined(ARDUINO)
  (void)modifiers;
  (void)hid_keys;
  return false;
#else
  if (g_ble_keyboard == nullptr || !g_ble_keyboard->isConnected()) {
    return false;
  }

  KeyReport report = {0};
  report.modifiers = modifiers;

  std::size_t idx = 0;
  for (std::uint8_t code : hid_keys) {
    std::uint8_t key = code;
    if (key & 0x80) {
      report.modifiers |= 0x02;
      key &= 0x7F;
    }

    if (key == 0) {
      continue;
    }

    report.keys[idx++] = key;
    if (idx >= 6) {
      break;
    }
  }

  g_ble_keyboard->sendReport(&report);

  KeyReport release_report = {0};
  g_ble_keyboard->sendReport(&release_report);
  return true;
#endif
}
