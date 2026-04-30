#include <unity.h>

#include "app/nmea_parser.h"

namespace {
constexpr double kEps = 1e-6;

void assert_state_equals(const GnssState& expected, const GnssState& actual) {
  TEST_ASSERT_EQUAL(expected.fix_valid, actual.fix_valid);
  TEST_ASSERT_EQUAL(expected.cog_valid, actual.cog_valid);
  TEST_ASSERT_EQUAL(expected.utc_valid, actual.utc_valid);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, expected.latitude, actual.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, expected.longitude, actual.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, expected.altitude_m, actual.altitude_m);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, expected.cog_deg, actual.cog_deg);
  TEST_ASSERT_EQUAL_UINT8(expected.sats, actual.sats);
  TEST_ASSERT_EQUAL_UINT32(expected.last_update_ms, actual.last_update_ms);
  TEST_ASSERT_EQUAL_UINT8(expected.utc_hour, actual.utc_hour);
  TEST_ASSERT_EQUAL_UINT8(expected.utc_min, actual.utc_min);
  TEST_ASSERT_EQUAL_UINT8(expected.utc_sec, actual.utc_sec);
}
}

void test_parse_gga_updates_fix_position_altitude_and_satellites() {
  GnssState state{};

  const bool ok = parseNmeaSentence(
      "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47", 1000,
      state);

  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(state.fix_valid);
  TEST_ASSERT_TRUE(state.utc_valid);
  TEST_ASSERT_EQUAL_UINT8(12, state.utc_hour);
  TEST_ASSERT_EQUAL_UINT8(35, state.utc_min);
  TEST_ASSERT_EQUAL_UINT8(19, state.utc_sec);
  TEST_ASSERT_EQUAL_UINT8(8, state.sats);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, 48.1173, state.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, 11.5166667, state.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, 545.4, state.altitude_m);
  TEST_ASSERT_EQUAL_UINT32(1000, state.last_update_ms);
}

void test_parse_rmc_updates_utc_and_cog() {
  GnssState state{};

  const bool ok = parseNmeaSentence(
      "$GPRMC,092204.999,A,4250.5589,N,08339.4335,W,000.0,054.7,191194,020.3,E*68", 2000,
      state);

  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(state.fix_valid);
  TEST_ASSERT_TRUE(state.cog_valid);
  TEST_ASSERT_TRUE(state.utc_valid);
  TEST_ASSERT_EQUAL_UINT8(9, state.utc_hour);
  TEST_ASSERT_EQUAL_UINT8(22, state.utc_min);
  TEST_ASSERT_EQUAL_UINT8(4, state.utc_sec);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, 54.7, state.cog_deg);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, 42.8426483, state.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, -83.657225, state.longitude);
  TEST_ASSERT_EQUAL_UINT32(2000, state.last_update_ms);
}

void test_parse_rmc_void_status_clears_fix_and_cog_valid() {
  GnssState state{};
  state.fix_valid = true;
  state.cog_valid = true;

  const bool ok = parseNmeaSentence(
      "$GPRMC,092205.000,V,4250.5589,N,08339.4335,W,000.0,054.7,191194,020.3,E*53", 3000,
      state);

  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_FALSE(state.fix_valid);
  TEST_ASSERT_FALSE(state.cog_valid);
  TEST_ASSERT_TRUE(state.utc_valid);
  TEST_ASSERT_EQUAL_UINT32(3000, state.last_update_ms);
}

void test_parse_unknown_sentence_returns_false_and_keeps_state() {
  GnssState state{};
  state.fix_valid = true;
  state.latitude = 1.0;

  const bool ok =
      parseNmeaSentence("$GPTXT,01,01,02,u-blox ag - www.u-blox.com*50", 4000, state);

  TEST_ASSERT_FALSE(ok);
  TEST_ASSERT_TRUE(state.fix_valid);
  TEST_ASSERT_DOUBLE_WITHIN(kEps, 1.0, state.latitude);
  TEST_ASSERT_EQUAL_UINT32(0, state.last_update_ms);
}

void test_parse_gga_with_non_numeric_utc_fails_and_keeps_state() {
  GnssState state{};
  state.fix_valid = true;
  state.cog_valid = true;
  state.utc_valid = true;
  state.latitude = 42.1;
  state.longitude = -83.6;
  state.altitude_m = 123.4;
  state.cog_deg = 12.3;
  state.sats = 9;
  state.last_update_ms = 999;
  state.utc_hour = 11;
  state.utc_min = 22;
  state.utc_sec = 33;
  const GnssState expected = state;

  const bool ok = parseNmeaSentence(
      "$GPGGA,12AA19,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00", 5000,
      state);

  TEST_ASSERT_FALSE(ok);
  assert_state_equals(expected, state);
}

void test_parse_gga_with_utc_trailing_char_fails_and_keeps_utc_state() {
  GnssState state{};
  state.utc_valid = true;
  state.utc_hour = 21;
  state.utc_min = 22;
  state.utc_sec = 23;
  const GnssState expected = state;

  const bool ok = parseNmeaSentence(
      "$GPGGA,123519X,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00", 5100,
      state);

  TEST_ASSERT_FALSE(ok);
  assert_state_equals(expected, state);
}

void test_parse_rmc_with_non_numeric_latitude_fails_and_keeps_state() {
  GnssState state{};
  state.fix_valid = true;
  state.cog_valid = true;
  state.utc_valid = true;
  state.latitude = 48.0;
  state.longitude = 11.0;
  state.altitude_m = 88.8;
  state.cog_deg = 90.0;
  state.sats = 7;
  state.last_update_ms = 1001;
  state.utc_hour = 1;
  state.utc_min = 2;
  state.utc_sec = 3;
  const GnssState expected = state;

  const bool ok = parseNmeaSentence(
      "$GPRMC,092204.999,A,42AB.5589,N,08339.4335,W,000.0,054.7,191194,020.3,E*00", 6000,
      state);

  TEST_ASSERT_FALSE(ok);
  assert_state_equals(expected, state);
}

void test_parse_gga_with_invalid_hemisphere_fails_and_keeps_state() {
  GnssState state{};
  state.fix_valid = true;
  state.cog_valid = false;
  state.utc_valid = true;
  state.latitude = -1.2;
  state.longitude = 3.4;
  state.altitude_m = 5.6;
  state.cog_deg = 7.8;
  state.sats = 4;
  state.last_update_ms = 2002;
  state.utc_hour = 4;
  state.utc_min = 5;
  state.utc_sec = 6;
  const GnssState expected = state;

  const bool ok = parseNmeaSentence(
      "$GPGGA,123519,4807.038,X,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00", 7000,
      state);

  TEST_ASSERT_FALSE(ok);
  assert_state_equals(expected, state);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_parse_gga_updates_fix_position_altitude_and_satellites);
  RUN_TEST(test_parse_rmc_updates_utc_and_cog);
  RUN_TEST(test_parse_rmc_void_status_clears_fix_and_cog_valid);
  RUN_TEST(test_parse_unknown_sentence_returns_false_and_keeps_state);
  RUN_TEST(test_parse_gga_with_non_numeric_utc_fails_and_keeps_state);
  RUN_TEST(test_parse_gga_with_utc_trailing_char_fails_and_keeps_utc_state);
  RUN_TEST(test_parse_rmc_with_non_numeric_latitude_fails_and_keeps_state);
  RUN_TEST(test_parse_gga_with_invalid_hemisphere_fails_and_keeps_state);

  return UNITY_END();
}
