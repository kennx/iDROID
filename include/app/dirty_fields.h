#pragma once

#include "app/gnss_state.h"

struct DirtyFields {
  bool status = false;
  bool latitude = false;
  bool longitude = false;
  bool altitude = false;
  bool cog = false;
  bool heading = false;
  bool utc = false;
};

DirtyFields computeDirtyFields(const GnssState& previous, const GnssState& current);
