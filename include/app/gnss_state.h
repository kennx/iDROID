#pragma once

#include <cstdint>

struct GnssState {
  bool fix_valid = false;
  bool cog_valid = false;
  bool utc_valid = false;
  double latitude = 0.0;
  double longitude = 0.0;
  double altitude_m = 0.0;
  double cog_deg = 0.0;
  std::uint8_t sats = 0;
  std::uint32_t last_update_ms = 0;
  std::uint8_t utc_hour = 0;
  std::uint8_t utc_min = 0;
  std::uint8_t utc_sec = 0;
};
