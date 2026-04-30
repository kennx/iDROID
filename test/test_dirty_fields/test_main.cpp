#include <unity.h>

#include "app/dirty_fields.h"

void test_same_state_has_no_dirty_fields() {
  GnssState previous{};
  GnssState current{};

  DirtyFields dirty = computeDirtyFields(previous, current);

  TEST_ASSERT_FALSE(dirty.status);
  TEST_ASSERT_FALSE(dirty.latitude);
  TEST_ASSERT_FALSE(dirty.longitude);
  TEST_ASSERT_FALSE(dirty.altitude);
  TEST_ASSERT_FALSE(dirty.cog);
  TEST_ASSERT_FALSE(dirty.heading);
  TEST_ASSERT_FALSE(dirty.utc);
}

void test_latitude_threshold_is_1e_minus_6() {
  GnssState previous{};
  GnssState current{};
  current.latitude = 1e-6;

  DirtyFields dirty = computeDirtyFields(previous, current);

  TEST_ASSERT_FALSE(dirty.latitude);

  current.latitude = 1.0001e-6;
  dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_TRUE(dirty.latitude);
}

void test_cog_wraparound_uses_min_circular_delta() {
  GnssState previous{};
  GnssState current{};

  previous.cog_deg = 359.95;
  current.cog_deg = 0.05;

  DirtyFields dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_FALSE(dirty.cog);

  previous.cog_deg = 359.94;
  current.cog_deg = 0.05;
  dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_TRUE(dirty.cog);
}

void test_numeric_thresholds_and_heading_behavior() {
  GnssState previous{};
  GnssState current{};

  current.longitude = 1e-6;
  current.altitude_m = 0.1;
  current.cog_deg = 0.1;

  DirtyFields dirty = computeDirtyFields(previous, current);

  TEST_ASSERT_FALSE(dirty.longitude);
  TEST_ASSERT_FALSE(dirty.altitude);
  TEST_ASSERT_FALSE(dirty.cog);
  TEST_ASSERT_FALSE(dirty.heading);

  current.longitude = 1.0001e-6;
  current.altitude_m = 0.1001;
  current.cog_deg = 0.1001;

  dirty = computeDirtyFields(previous, current);

  TEST_ASSERT_TRUE(dirty.longitude);
  TEST_ASSERT_TRUE(dirty.altitude);
  TEST_ASSERT_TRUE(dirty.cog);
  TEST_ASSERT_FALSE(dirty.heading);
}

void test_status_and_utc_changes_are_tracked() {
  GnssState previous{};
  GnssState current{};

  current.fix_valid = true;
  current.sats = 7;
  DirtyFields dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_TRUE(dirty.status);

  previous = current;
  current.utc_valid = true;
  current.utc_hour = 12;
  current.utc_min = 34;
  current.utc_sec = 56;
  dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_TRUE(dirty.utc);
}

void test_altitude_dirty_when_fix_valid_toggles() {
  GnssState previous{};
  GnssState current{};
  previous.fix_valid = false;
  previous.altitude_m = 123.4;
  current.fix_valid = true;
  current.altitude_m = 123.4;

  DirtyFields dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_TRUE(dirty.altitude);
}

void test_cog_dirty_when_cog_valid_toggles() {
  GnssState previous{};
  GnssState current{};
  previous.cog_valid = false;
  previous.cog_deg = 77.7;
  current.cog_valid = true;
  current.cog_deg = 77.7;

  DirtyFields dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_TRUE(dirty.cog);
}

void test_status_ignores_last_update_ms_only_change() {
  GnssState previous{};
  GnssState current{};
  previous.fix_valid = true;
  previous.cog_valid = true;
  previous.sats = 5;
  previous.last_update_ms = 1000;
  current = previous;
  current.last_update_ms = 1001;

  DirtyFields dirty = computeDirtyFields(previous, current);
  TEST_ASSERT_FALSE(dirty.status);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  RUN_TEST(test_same_state_has_no_dirty_fields);
  RUN_TEST(test_latitude_threshold_is_1e_minus_6);
  RUN_TEST(test_numeric_thresholds_and_heading_behavior);
  RUN_TEST(test_cog_wraparound_uses_min_circular_delta);
  RUN_TEST(test_status_and_utc_changes_are_tracked);
  RUN_TEST(test_altitude_dirty_when_fix_valid_toggles);
  RUN_TEST(test_cog_dirty_when_cog_valid_toggles);
  RUN_TEST(test_status_ignores_last_update_ms_only_change);

  return UNITY_END();
}
