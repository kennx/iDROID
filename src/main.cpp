#include <Arduino.h>
#include <M5Unified.h>

#include "app/nmea_parser.h"
#include "app/ui_renderer.h"

namespace {
constexpr std::uint32_t kRenderTickMs = 100;
constexpr std::uint32_t kStaleTimeoutMs = 2000;
constexpr std::size_t kNmeaBufferSize = 160;
constexpr std::size_t kMaxSerialBytesPerLoop = 128;
constexpr int kLineHeight = 18;
constexpr int kGnssUartRxPin = 15;
constexpr int kGnssUartTxPin = 13;

GnssState g_state{};
UiRenderer g_renderer{};
std::uint32_t g_last_render_ms = 0;

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
}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  Serial1.begin(115200, SERIAL_8N1, kGnssUartRxPin, kGnssUartTxPin);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.fillScreen(TFT_BLACK);
  g_renderer.begin();
  g_last_render_ms = millis();
}

void loop() {
  M5.update();

  const std::uint32_t now_ms = millis();

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

  if ((now_ms - g_last_render_ms) >= kRenderTickMs) {
    g_last_render_ms = now_ms;
    const RenderFrame frame = g_renderer.renderTick(now_ms, g_state);
    drawFrame(frame);
  }
}
