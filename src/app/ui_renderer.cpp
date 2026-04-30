#include "app/ui_renderer.h"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace {
template <std::size_t N>
std::string format_line(const char* fmt, ...) {
  std::array<char, N> buf{};
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf.data(), buf.size(), fmt, args);
  va_end(args);
  return std::string(buf.data());
}

DirtyFields all_dirty() {
  DirtyFields dirty{};
  dirty.status = true;
  dirty.latitude = true;
  dirty.longitude = true;
  dirty.altitude = true;
  dirty.cog = true;
  dirty.heading = true;
  dirty.utc = true;
  return dirty;
}
}  // namespace

std::string formatLatitude(double latitude) { return format_line<13>("LAT:%+08.4f", latitude); }

std::string formatLongitude(double longitude) { return format_line<14>("LON:%+08.4f", longitude); }

std::string formatAltitude(bool fix_valid, double altitude_m) {
  if (!fix_valid) {
    return "ALT:N/A     ";
  }
  return format_line<13>("ALT:%07.1fm", altitude_m);
}

std::string formatCog(bool cog_valid, double cog_deg) {
  if (!cog_valid) {
    return "COG:N/A    ";
  }
  return format_line<13>("COG:%07.1f", cog_deg);
}

std::string formatUtc(bool utc_valid, std::uint8_t hour, std::uint8_t minute, std::uint8_t second) {
  if (!utc_valid) {
    return "UTC:--:--:--";
  }
  return format_line<13>("UTC:%02u:%02u:%02u", hour, minute, second);
}

void UiRenderer::begin() {
  has_previous_ = false;
  previous_state_ = GnssState{};
  frame_ = RenderFrame{};
}

RenderFrame UiRenderer::renderTick(std::uint32_t now_ms, const GnssState& state) {
  (void)now_ms;

  if (!has_previous_) {
    frame_.dirty = all_dirty();
  } else {
    frame_.dirty = computeDirtyFields(previous_state_, state);
  }

  if (state.fix_valid) {
    frame_.status = format_line<12>("FIX:%c SAT:%02u", 'Y', state.sats);
    frame_.latitude = formatLatitude(state.latitude);
    frame_.longitude = formatLongitude(state.longitude);
    frame_.altitude = formatAltitude(state.fix_valid, state.altitude_m);
  } else {
    frame_.status = format_line<12>("FIX:%c SAT:%02u", 'N', state.sats);
    frame_.latitude = formatLatitude(state.latitude);
    frame_.longitude = formatLongitude(state.longitude);
    frame_.altitude = formatAltitude(state.fix_valid, state.altitude_m);
  }

  frame_.cog = formatCog(state.cog_valid, state.cog_deg);

  frame_.heading = "HDG:N/A     ";

  frame_.utc = formatUtc(state.utc_valid, state.utc_hour, state.utc_min, state.utc_sec);

  previous_state_ = state;
  has_previous_ = true;

  return frame_;
}
