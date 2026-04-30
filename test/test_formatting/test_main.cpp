#include <unity.h>

#include <cstddef>

#include "app/ui_renderer.h"

void test_public_format_helpers_are_callable() {
  TEST_ASSERT_EQUAL_STRING("LAT:-01.2000", formatLatitude(-1.2).c_str());
  TEST_ASSERT_EQUAL_STRING("LON:+03.4000", formatLongitude(3.4).c_str());
  TEST_ASSERT_EQUAL_STRING("LON:+180.0000", formatLongitude(180.0).c_str());
  TEST_ASSERT_EQUAL_STRING("LON:-180.0000", formatLongitude(-180.0).c_str());
  TEST_ASSERT_EQUAL_STRING("ALT:00005.6m", formatAltitude(true, 5.6).c_str());
  TEST_ASSERT_EQUAL_STRING("ALT:N/A     ", formatAltitude(false, 5.6).c_str());
  TEST_ASSERT_EQUAL_STRING("COG:00007.8", formatCog(true, 7.8).c_str());
  TEST_ASSERT_EQUAL_STRING("COG:N/A    ", formatCog(false, 7.8).c_str());
  TEST_ASSERT_EQUAL_STRING("UTC:01:02:03", formatUtc(true, 1, 2, 3).c_str());
  TEST_ASSERT_EQUAL_STRING("UTC:--:--:--", formatUtc(false, 1, 2, 3).c_str());
}

void test_begin_and_first_render_marks_all_fields_dirty() {
  UiRenderer renderer;
  renderer.begin();

  GnssState state{};
  state.fix_valid = true;
  state.cog_valid = true;
  state.utc_valid = true;
  state.latitude = 31.230416;
  state.longitude = 121.473701;
  state.altitude_m = 4.2;
  state.cog_deg = 278.4;
  state.sats = 9;
  state.last_update_ms = 1000;
  state.utc_hour = 8;
  state.utc_min = 1;
  state.utc_sec = 2;

  RenderFrame frame = renderer.renderTick(1050, state);

  TEST_ASSERT_TRUE(frame.dirty.status);
  TEST_ASSERT_TRUE(frame.dirty.latitude);
  TEST_ASSERT_TRUE(frame.dirty.longitude);
  TEST_ASSERT_TRUE(frame.dirty.altitude);
  TEST_ASSERT_TRUE(frame.dirty.cog);
  TEST_ASSERT_TRUE(frame.dirty.heading);
  TEST_ASSERT_TRUE(frame.dirty.utc);
}

void test_render_uses_compute_dirty_fields_thresholds() {
  UiRenderer renderer;
  renderer.begin();

  GnssState base{};
  base.fix_valid = true;
  base.cog_valid = true;
  base.utc_valid = true;
  base.latitude = 48.1173000;
  base.longitude = 11.5166667;
  base.altitude_m = 0.0;
  base.cog_deg = 10.0;
  base.sats = 8;
  base.last_update_ms = 1000;
  base.utc_hour = 12;
  base.utc_min = 35;
  base.utc_sec = 19;
  (void)renderer.renderTick(1000, base);

  GnssState tiny_change = base;
  tiny_change.latitude += 1e-6;
  tiny_change.longitude += 1e-6;
  tiny_change.altitude_m += 0.1;
  tiny_change.cog_deg += 0.1;

  RenderFrame frame = renderer.renderTick(1010, tiny_change);

  TEST_ASSERT_FALSE(frame.dirty.latitude);
  TEST_ASSERT_FALSE(frame.dirty.longitude);
  TEST_ASSERT_FALSE(frame.dirty.altitude);
  TEST_ASSERT_FALSE(frame.dirty.cog);
}

void test_fixed_width_formatting_and_heading_na() {
  UiRenderer renderer;
  renderer.begin();

  GnssState state{};
  state.fix_valid = true;
  state.cog_valid = true;
  state.utc_valid = true;
  state.latitude = -1.2;
  state.longitude = 3.4;
  state.altitude_m = 5.6;
  state.cog_deg = 7.8;
  state.sats = 4;
  state.last_update_ms = 2222;
  state.utc_hour = 1;
  state.utc_min = 2;
  state.utc_sec = 3;

  RenderFrame valid_frame = renderer.renderTick(2300, state);

  const std::size_t status_len = valid_frame.status.size();
  const std::size_t lat_len = valid_frame.latitude.size();
  const std::size_t lon_len = valid_frame.longitude.size();
  const std::size_t alt_len = valid_frame.altitude.size();
  const std::size_t cog_len = valid_frame.cog.size();
  const std::size_t heading_len = valid_frame.heading.size();
  const std::size_t utc_len = valid_frame.utc.size();

  GnssState invalid = state;
  invalid.fix_valid = false;
  invalid.cog_valid = false;
  invalid.utc_valid = false;
  RenderFrame invalid_frame = renderer.renderTick(2400, invalid);

  TEST_ASSERT_EQUAL_UINT32(status_len, invalid_frame.status.size());
  TEST_ASSERT_EQUAL_UINT32(lat_len, invalid_frame.latitude.size());
  TEST_ASSERT_EQUAL_UINT32(lon_len, invalid_frame.longitude.size());
  TEST_ASSERT_EQUAL_UINT32(alt_len, invalid_frame.altitude.size());
  TEST_ASSERT_EQUAL_UINT32(cog_len, invalid_frame.cog.size());
  TEST_ASSERT_EQUAL_UINT32(heading_len, invalid_frame.heading.size());
  TEST_ASSERT_EQUAL_UINT32(utc_len, invalid_frame.utc.size());
  TEST_ASSERT_EQUAL_STRING("HDG:N/A     ", valid_frame.heading.c_str());
  TEST_ASSERT_EQUAL_STRING("HDG:N/A     ", invalid_frame.heading.c_str());
  TEST_ASSERT_EQUAL_STRING(formatLatitude(state.latitude).c_str(), valid_frame.latitude.c_str());
  TEST_ASSERT_EQUAL_STRING(formatLongitude(state.longitude).c_str(), valid_frame.longitude.c_str());
  TEST_ASSERT_EQUAL_STRING(formatAltitude(state.fix_valid, state.altitude_m).c_str(),
                           valid_frame.altitude.c_str());
  TEST_ASSERT_EQUAL_STRING(formatCog(state.cog_valid, state.cog_deg).c_str(), valid_frame.cog.c_str());
  TEST_ASSERT_EQUAL_STRING(
      formatUtc(state.utc_valid, state.utc_hour, state.utc_min, state.utc_sec).c_str(),
      valid_frame.utc.c_str());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_public_format_helpers_are_callable);
  RUN_TEST(test_begin_and_first_render_marks_all_fields_dirty);
  RUN_TEST(test_render_uses_compute_dirty_fields_thresholds);
  RUN_TEST(test_fixed_width_formatting_and_heading_na);

  return UNITY_END();
}
