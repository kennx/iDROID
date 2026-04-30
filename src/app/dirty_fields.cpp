#include "app/dirty_fields.h"

#include <cmath>

namespace {
constexpr double kLatLonThreshold = 1e-6;
constexpr double kAltThreshold = 0.1;
constexpr double kCogThreshold = 0.1;
constexpr double kThresholdEpsilon = 1e-12;

double circular_angle_delta_deg(double a_deg, double b_deg) {
  double delta = std::fmod(std::fabs(a_deg - b_deg), 360.0);
  if (delta > 180.0) {
    delta = 360.0 - delta;
  }
  return delta;
}
}  // namespace

DirtyFields computeDirtyFields(const GnssState& previous, const GnssState& current) {
  DirtyFields dirty{};

  dirty.status = previous.fix_valid != current.fix_valid || previous.cog_valid != current.cog_valid ||
                 previous.sats != current.sats;

  dirty.latitude = std::fabs(previous.latitude - current.latitude) > kLatLonThreshold;
  dirty.longitude = std::fabs(previous.longitude - current.longitude) > kLatLonThreshold;
  dirty.altitude = previous.fix_valid != current.fix_valid ||
                   std::fabs(previous.altitude_m - current.altitude_m) > kAltThreshold;
  dirty.cog = previous.cog_valid != current.cog_valid ||
              circular_angle_delta_deg(previous.cog_deg, current.cog_deg) >
              (kCogThreshold + kThresholdEpsilon);

  dirty.heading = false;

  dirty.utc = previous.utc_valid != current.utc_valid || previous.utc_hour != current.utc_hour ||
              previous.utc_min != current.utc_min || previous.utc_sec != current.utc_sec;

  return dirty;
}
