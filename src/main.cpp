#include <Arduino.h>
#include <M5Unified.h>
#include <M5Cardputer.h>

#include "app/nmea_parser.h"
#include "app/bt_keyboard.h"
#include "app/ui_renderer.h"
#include "app/view_nav.h"

namespace {
constexpr std::uint32_t kRenderTickMs = 100;
constexpr std::uint32_t kStaleTimeoutMs = 2000;
constexpr std::size_t kNmeaBufferSize = 160;
constexpr std::size_t kMaxSerialBytesPerLoop = 128;
constexpr int kLineHeight = 18;
constexpr int kHeaderY = 8;
constexpr int kMenuStartY = 40;
constexpr int kMenuLineHeight = 20;
constexpr int kGnssUartRxPin = 15;
constexpr int kGnssUartTxPin = 13;
constexpr const char* kBtKeyboardDeviceName = "iDROID-KBD";
constexpr std::uint8_t kNavItemCount = 2;
constexpr std::uint8_t kNavItemGnss = 0;
constexpr std::uint8_t kNavItemBtKeyboard = 1;

GnssState g_state{};
UiRenderer g_renderer{};
std::uint32_t g_last_render_ms = 0;
ViewId g_view = ViewId::kNav;
std::uint8_t g_nav_index = 0;
bool g_force_full_redraw = true;
BtKbdState g_bt_kbd_state{};
BtKbdUiState g_last_bt_kbd_ui = BtKbdUiState::kIdle;
std::uint32_t g_bt_key_event_count = 0;
std::uint32_t g_bt_send_ok_count = 0;
std::uint32_t g_bt_send_fail_count = 0;
char g_bt_last_char = '-';
bool g_bt_probe_sent = false;

char g_nmea_buffer[kNmeaBufferSize] = {0};
std::size_t g_nmea_length = 0;

void feedNmeaByte(char ch, std::uint32_t now_ms) {
  if (ch == '\r') {
    return;
  }

  if (ch == '\n') {
    if (g_nmea_length > 0) {
      g_nmea_buffer[g_nmea_length] = '\0';
      (void)parseNmeaSentence(g_nmea_buffer, now_ms, g_state);
      g_nmea_length = 0;
    }
    return;
  }

  if (g_nmea_length < (kNmeaBufferSize - 1)) {
    g_nmea_buffer[g_nmea_length++] = ch;
    return;
  }

  g_nmea_length = 0;
}

void drawLineIfDirty(bool dirty, int y, const std::string& text) {
  if (!dirty) {
    return;
  }

  M5.Display.setCursor(0, y + 2);
  M5.Display.print(text.c_str());
}

void drawFrame(const RenderFrame& frame) {
  M5.Display.startWrite();
  drawLineIfDirty(frame.dirty.status, 0 * kLineHeight, frame.status);
  drawLineIfDirty(frame.dirty.latitude, 1 * kLineHeight, frame.latitude);
  drawLineIfDirty(frame.dirty.longitude, 2 * kLineHeight, frame.longitude);
  drawLineIfDirty(frame.dirty.altitude, 3 * kLineHeight, frame.altitude);
  drawLineIfDirty(frame.dirty.cog, 4 * kLineHeight, frame.cog);
  drawLineIfDirty(frame.dirty.heading, 5 * kLineHeight, frame.heading);
  drawLineIfDirty(frame.dirty.utc, 6 * kLineHeight, frame.utc);
  M5.Display.endWrite();
}

void clearAndPrepareTextScreen() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 0);
}

void drawNavPage() {
  M5.Display.startWrite();
  M5.Display.setCursor(0, kHeaderY);
  M5.Display.print("iDROID");

  M5.Display.setCursor(0, kMenuStartY);
  M5.Display.print("MENU");

  M5.Display.setCursor(0, kMenuStartY + kMenuLineHeight);
  if (g_nav_index == kNavItemGnss) {
    M5.Display.print("> GNSS");
  } else {
    M5.Display.print("  GNSS");
  }

  M5.Display.setCursor(0, kMenuStartY + 2 * kMenuLineHeight);
  if (g_nav_index == kNavItemBtKeyboard) {
    M5.Display.print("> BT Keyboard");
  } else {
    M5.Display.print("  BT Keyboard");
  }

  M5.Display.setCursor(0, kMenuStartY + 3 * kMenuLineHeight);
  M5.Display.print("Enter: Open");
  M5.Display.endWrite();
}

const char* btKbdStatusText(BtKbdUiState ui) {
  switch (ui) {
    case BtKbdUiState::kIdle:
      return "IDLE";
    case BtKbdUiState::kAdvertising:
      return "ADVERTISING";
    case BtKbdUiState::kConnected:
      return "CONNECTED";
    case BtKbdUiState::kDisconnected:
      return "DISCONNECTED";
    case BtKbdUiState::kInitFailed:
      return "INIT FAILED";
  }
  return "UNKNOWN";
}

void drawBtKeyboardPage() {
  M5.Display.startWrite();
  M5.Display.setCursor(0, kHeaderY);
  M5.Display.print("BT Keyboard");

  M5.Display.setCursor(0, kMenuStartY);
  M5.Display.print("NAME: iDROID-KBD");

  M5.Display.setCursor(0, kMenuStartY + kMenuLineHeight);
  M5.Display.printf("STATE: %s", btKbdStatusText(g_bt_kbd_state.ui));

  M5.Display.setCursor(0, kMenuStartY + 2 * kMenuLineHeight);
  M5.Display.print("BtnGO: Back");

  M5.Display.setCursor(0, kMenuStartY + 3 * kMenuLineHeight);
  M5.Display.print("Type on keyboard");

  M5.Display.setCursor(0, kMenuStartY + 4 * kMenuLineHeight);
  M5.Display.printf("EV:%lu OK:%lu", static_cast<unsigned long>(g_bt_key_event_count),
                    static_cast<unsigned long>(g_bt_send_ok_count));

  M5.Display.setCursor(0, kMenuStartY + 5 * kMenuLineHeight);
  M5.Display.printf("FAIL:%lu LAST:%c", static_cast<unsigned long>(g_bt_send_fail_count),
                    g_bt_last_char);
  M5.Display.endWrite();
}

bool isNavUpPressed(const Keyboard_Class::KeysState& keys) {
  return keys.fn && M5Cardputer.Keyboard.isKeyPressed(';');
}

bool isNavDownPressed(const Keyboard_Class::KeysState& keys) {
  return keys.fn && M5Cardputer.Keyboard.isKeyPressed('.');
}

void handleNavInput() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();

  const bool up_pressed = isNavUpPressed(keys);
  const bool down_pressed = isNavDownPressed(keys);
  if (up_pressed) {
    g_nav_index = moveNavSelection(g_nav_index, -1, kNavItemCount);
    g_force_full_redraw = true;
  } else if (down_pressed) {
    g_nav_index = moveNavSelection(g_nav_index, +1, kNavItemCount);
    g_force_full_redraw = true;
  }

  const ViewId next = nextViewFromNavSelection(g_nav_index, keys.enter);
  if (next != g_view) {
    if (next == ViewId::kBtKeyboard) {
      const bool started = btKbdStartService(kBtKeyboardDeviceName);
      g_bt_kbd_state = enterBtKbd(started);
      g_last_bt_kbd_ui = BtKbdUiState::kIdle;
      g_bt_key_event_count = 0;
      g_bt_send_ok_count = 0;
      g_bt_send_fail_count = 0;
      g_bt_last_char = '-';
      g_bt_probe_sent = false;
    }
    g_view = next;
    g_force_full_redraw = true;
    if (next == ViewId::kGnss) {
      g_renderer.begin();
    }
  }
}

void handleGnssInput() {
  const ViewId next = nextView(g_view, false, M5.BtnA.wasPressed());
  if (next != g_view) {
    g_view = next;
    g_force_full_redraw = true;
  }
}

void sendBtKeyboardInput(const Keyboard_Class::KeysState& keys) {
  if (!keys.hid_keys.empty()) {
    const bool ok = btKbdSendHidStroke(keys.modifiers, keys.hid_keys);
    g_bt_last_char = 'H';
    if (ok) {
      ++g_bt_send_ok_count;
    } else {
      ++g_bt_send_fail_count;
    }
  } else {
    for (char ch : keys.word) {
      const bool ok = btKbdSendChar(ch);
      g_bt_last_char = ch;
      if (ok) {
        ++g_bt_send_ok_count;
      } else {
        ++g_bt_send_fail_count;
      }
    }
  }

  if (keys.enter) {
    const bool ok = btKbdSendSpecial(BtKbdSpecialKey::kEnter);
    g_bt_last_char = 'E';
    if (ok) {
      ++g_bt_send_ok_count;
    } else {
      ++g_bt_send_fail_count;
    }
  }
  if (keys.del) {
    const bool ok = btKbdSendSpecial(BtKbdSpecialKey::kBackspace);
    g_bt_last_char = 'B';
    if (ok) {
      ++g_bt_send_ok_count;
    } else {
      ++g_bt_send_fail_count;
    }
  }
  if (keys.tab) {
    const bool ok = btKbdSendSpecial(BtKbdSpecialKey::kTab);
    g_bt_last_char = 'T';
    if (ok) {
      ++g_bt_send_ok_count;
    } else {
      ++g_bt_send_fail_count;
    }
  }
  if (keys.space && keys.word.empty()) {
    const bool ok = btKbdSendSpecial(BtKbdSpecialKey::kSpace);
    g_bt_last_char = 'S';
    if (ok) {
      ++g_bt_send_ok_count;
    } else {
      ++g_bt_send_fail_count;
    }
  }
}

void handleBtKeyboardInput() {
  if (M5.BtnA.wasPressed()) {
    btKbdStopService();
    g_bt_kbd_state = exitBtKbd(g_bt_kbd_state);
    g_view = ViewId::kNav;
    g_force_full_redraw = true;
    return;
  }

  g_bt_kbd_state = btKbdSyncConnection(g_bt_kbd_state);
  if (g_bt_kbd_state.ui != g_last_bt_kbd_ui) {
    g_last_bt_kbd_ui = g_bt_kbd_state.ui;
    g_force_full_redraw = true;
  }

  if (!g_bt_probe_sent && g_bt_kbd_state.ui == BtKbdUiState::kConnected) {
    const bool p1 = btKbdSendChar('[');
    const bool p2 = btKbdSendChar('B');
    const bool p3 = btKbdSendChar('T');
    const bool p4 = btKbdSendChar(']');
    const bool p5 = btKbdSendSpecial(BtKbdSpecialKey::kSpace);
    const bool p6 = btKbdSendChar('O');
    const bool p7 = btKbdSendChar('K');
    const bool p8 = btKbdSendSpecial(BtKbdSpecialKey::kEnter);

    const bool probe_ok = p1 && p2 && p3 && p4 && p5 && p6 && p7 && p8;
    if (probe_ok) {
      g_bt_send_ok_count += 8;
      g_bt_last_char = 'P';
    } else {
      g_bt_send_fail_count += 8;
      g_bt_last_char = 'F';
    }
    g_bt_probe_sent = true;
    g_force_full_redraw = true;
  }

  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  if (!canSendBtKbdInput(g_bt_kbd_state)) {
    return;
  }

  Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
  ++g_bt_key_event_count;
  sendBtKeyboardInput(keys);
  g_force_full_redraw = true;
}

void processGnssData(std::uint32_t now_ms) {
  std::size_t processed_bytes = 0;
  while (Serial1.available() > 0 && processed_bytes < kMaxSerialBytesPerLoop) {
    const char ch = static_cast<char>(Serial1.read());
    feedNmeaByte(ch, now_ms);
    ++processed_bytes;
  }

  if (g_state.last_update_ms > 0 && (now_ms - g_state.last_update_ms) >= kStaleTimeoutMs) {
    g_state.fix_valid = false;
    g_state.cog_valid = false;
  }
}

void renderGnssTick(std::uint32_t now_ms) {
  if ((now_ms - g_last_render_ms) < kRenderTickMs) {
    return;
  }
  g_last_render_ms = now_ms;
  const RenderFrame frame = g_renderer.renderTick(now_ms, g_state);
  drawFrame(frame);
}
}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5Cardputer.begin(cfg);

  Serial1.begin(115200, SERIAL_8N1, kGnssUartRxPin, kGnssUartTxPin);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.fillScreen(TFT_BLACK);
  g_renderer.begin();
  g_last_render_ms = millis();
}

void loop() {
  M5Cardputer.update();

  const std::uint32_t now_ms = millis();

  if (g_view == ViewId::kNav) {
    handleNavInput();
  } else if (g_view == ViewId::kGnss) {
    handleGnssInput();
    processGnssData(now_ms);
  } else {
    handleBtKeyboardInput();
  }

  if (g_force_full_redraw) {
    clearAndPrepareTextScreen();
    if (g_view == ViewId::kNav) {
      drawNavPage();
    } else if (g_view == ViewId::kBtKeyboard) {
      drawBtKeyboardPage();
    }
    g_force_full_redraw = false;
  }

  if (g_view == ViewId::kGnss) {
    renderGnssTick(now_ms);
  }
}
