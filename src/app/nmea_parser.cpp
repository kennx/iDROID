#include "app/nmea_parser.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace {
constexpr int kMaxFields = 32;

int splitCsv(char* text, char* fields[], int max_fields) {
  int count = 0;
  char* p = text;

  while (count < max_fields) {
    fields[count++] = p;

    char* comma = std::strchr(p, ',');
    if (comma == nullptr) {
      break;
    }

    *comma = '\0';
    p = comma + 1;
  }

  return count;
}

bool parseUtc(const char* text, std::uint8_t& hour, std::uint8_t& min, std::uint8_t& sec) {
  if (text == nullptr || std::strlen(text) < 6) {
    return false;
  }

  for (int i = 0; i < 6; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
      return false;
    }
  }

  if (text[6] != '\0') {
    if (text[6] != '.') {
      return false;
    }

    if (text[7] == '\0') {
      return false;
    }

    for (int i = 7; text[i] != '\0'; ++i) {
      if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
        return false;
      }
    }
  }

  char hh[3] = {text[0], text[1], '\0'};
  char mm[3] = {text[2], text[3], '\0'};
  char ss[3] = {text[4], text[5], '\0'};

  const std::uint8_t parsed_hour = static_cast<std::uint8_t>(std::strtoul(hh, nullptr, 10));
  const std::uint8_t parsed_min = static_cast<std::uint8_t>(std::strtoul(mm, nullptr, 10));
  const std::uint8_t parsed_sec = static_cast<std::uint8_t>(std::strtoul(ss, nullptr, 10));

  if (parsed_hour > 23 || parsed_min > 59 || parsed_sec > 59) {
    return false;
  }

  hour = parsed_hour;
  min = parsed_min;
  sec = parsed_sec;
  return true;
}

bool parseLatLon(const char* value_text, const char* hemi_text, double& out_deg) {
  if (value_text == nullptr || value_text[0] == '\0' || hemi_text == nullptr || hemi_text[0] == '\0') {
    return false;
  }

  const char hemi = hemi_text[0];
  if (hemi != 'N' && hemi != 'S' && hemi != 'E' && hemi != 'W') {
    return false;
  }

  char* endptr = nullptr;
  const double value = std::strtod(value_text, &endptr);
  if (endptr == value_text || (endptr != nullptr && endptr[0] != '\0')) {
    return false;
  }

  const double degrees = static_cast<int>(value / 100.0);
  const double minutes = value - (degrees * 100.0);

  if (minutes < 0.0 || minutes >= 60.0) {
    return false;
  }

  double parsed_deg = degrees + (minutes / 60.0);

  if (hemi == 'S' || hemi == 'W') {
    parsed_deg = -parsed_deg;
  }

  out_deg = parsed_deg;

  return true;
}

bool parseGga(char* fields[], int field_count, std::uint32_t now_ms, GnssState& state) {
  if (field_count < 10) {
    return false;
  }

  GnssState next = state;

  std::uint8_t hour = 0;
  std::uint8_t min = 0;
  std::uint8_t sec = 0;
  if (!parseUtc(fields[1], hour, min, sec)) {
    return false;
  }
  next.utc_valid = true;
  next.utc_hour = hour;
  next.utc_min = min;
  next.utc_sec = sec;

  double latitude = 0.0;
  if (!parseLatLon(fields[2], fields[3], latitude)) {
    return false;
  }
  next.latitude = latitude;

  double longitude = 0.0;
  if (!parseLatLon(fields[4], fields[5], longitude)) {
    return false;
  }
  next.longitude = longitude;

  const int fix_quality = std::atoi(fields[6]);
  next.fix_valid = fix_quality > 0;
  next.sats = static_cast<std::uint8_t>(std::strtoul(fields[7], nullptr, 10));
  next.altitude_m = std::strtod(fields[9], nullptr);
  next.last_update_ms = now_ms;

  state = next;

  return true;
}

bool parseRmc(char* fields[], int field_count, std::uint32_t now_ms, GnssState& state) {
  if (field_count < 9) {
    return false;
  }

  GnssState next = state;

  std::uint8_t hour = 0;
  std::uint8_t min = 0;
  std::uint8_t sec = 0;
  if (!parseUtc(fields[1], hour, min, sec)) {
    return false;
  }
  next.utc_valid = true;
  next.utc_hour = hour;
  next.utc_min = min;
  next.utc_sec = sec;

  const bool active = fields[2] != nullptr && fields[2][0] == 'A';
  next.fix_valid = active;

  double latitude = 0.0;
  if (!parseLatLon(fields[3], fields[4], latitude)) {
    return false;
  }
  next.latitude = latitude;

  double longitude = 0.0;
  if (!parseLatLon(fields[5], fields[6], longitude)) {
    return false;
  }
  next.longitude = longitude;

  if (active && fields[8] != nullptr && fields[8][0] != '\0') {
    next.cog_deg = std::strtod(fields[8], nullptr);
    next.cog_valid = true;
  } else {
    next.cog_valid = false;
  }

  next.last_update_ms = now_ms;
  state = next;
  return true;
}
}  // namespace

bool parseNmeaSentence(const char* sentence, std::uint32_t now_ms, GnssState& state) {
  if (sentence == nullptr || sentence[0] != '$') {
    return false;
  }

  char buffer[160] = {0};
  std::strncpy(buffer, sentence, sizeof(buffer) - 1);

  char* checksum = std::strchr(buffer, '*');
  if (checksum != nullptr) {
    *checksum = '\0';
  }

  char* fields[kMaxFields] = {nullptr};
  const int field_count = splitCsv(buffer, fields, kMaxFields);
  if (field_count <= 0 || fields[0] == nullptr) {
    return false;
  }

  if (std::strcmp(fields[0], "$GPGGA") == 0 || std::strcmp(fields[0], "$GNGGA") == 0) {
    return parseGga(fields, field_count, now_ms, state);
  }

  if (std::strcmp(fields[0], "$GPRMC") == 0 || std::strcmp(fields[0], "$GNRMC") == 0) {
    return parseRmc(fields, field_count, now_ms, state);
  }

  return false;
}
