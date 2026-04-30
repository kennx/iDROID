#pragma once

#include <cstdint>
#include <string>

#include "app/dirty_fields.h"
#include "app/gnss_state.h"

struct RenderFrame {
  DirtyFields dirty{};
  std::string status;
  std::string latitude;
  std::string longitude;
  std::string altitude;
  std::string cog;
  std::string heading;
  std::string utc;
};

std::string formatLatitude(double latitude);
std::string formatLongitude(double longitude);
std::string formatAltitude(bool fix_valid, double altitude_m);
std::string formatCog(bool cog_valid, double cog_deg);
std::string formatUtc(bool utc_valid, std::uint8_t hour, std::uint8_t minute, std::uint8_t second);

class UiRenderer {
 public:
  void begin();
  RenderFrame renderTick(std::uint32_t now_ms, const GnssState& state);

 private:
  bool has_previous_ = false;
  GnssState previous_state_{};
  RenderFrame frame_{};
};
