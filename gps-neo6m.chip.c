// SPDX-License-Identifier: MIT
// Copyright (C) 2026 SmartFarm
//
// NEO-6M simulation for Wokwi: one $GPRMC and one $GPGGA sentence per second.

#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Development geofence center; the sliders move the simulated collar away from it.
static const double kBaseLatitude = -16.5;
static const double kBaseLongitude = -68.1;
static const double kMetersPerDegreeLatitude = 111320.0;
// cos(16.5 degrees), the base latitude, precomputed so the chip needs no math library.
static const double kCosBaseLatitude = 0.95881973;
// 2026-01-01T00:00:00Z in Unix seconds; the device uses GPS time only without Wi-Fi, so NTP wins in Wokwi.
static const uint64_t kStartEpochSeconds = 1767225600ULL;

enum { kBodyCapacity = 96, kFrameCapacity = 192 };

typedef struct {
  uart_dev_t uart;
  uint32_t north_attr;
  uint32_t east_attr;
  bool writing;
  char frame[kFrameCapacity];
} chip_state_t;

static chip_state_t chip;

// The next frame must not overwrite bytes that are still on the wire.
static void on_uart_write_done(void *user_data) {
  chip_state_t *state = (chip_state_t *)user_data;
  state->writing = false;
}

// Degrees become NMEA whole degrees plus decimal minutes.
static void split_degrees(double value, int *whole, double *minutes) {
  const double absolute = value < 0.0 ? -value : value;
  *whole = (int)absolute;
  *minutes = (absolute - (double)*whole) * 60.0;
}

// The receiver discards any sentence whose checksum does not match its body.
static int append_sentence(char *out, size_t size, const char *body) {
  unsigned char checksum = 0;
  for (const char *p = body; *p != '\0'; p++) {
    checksum ^= (unsigned char)*p;
  }
  return snprintf(out, size, "$%s*%02X\r\n", body, checksum);
}

// Hinnant's civil_from_days, written by hand so the chip relies only on the Wokwi API and stdio.
static void utc_from_epoch(uint64_t seconds, int *year, int *month, int *day, int *hour, int *minute, int *second) {
  const uint64_t day_count = seconds / 86400ULL;
  const uint64_t time_of_day = seconds % 86400ULL;
  *hour = (int)(time_of_day / 3600ULL);
  *minute = (int)((time_of_day % 3600ULL) / 60ULL);
  *second = (int)(time_of_day % 60ULL);

  const int64_t z = (int64_t)day_count + 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int64_t doe = z - era * 146097;
  const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int64_t mp = (5 * doy + 2) / 153;
  const int64_t d = doy - (153 * mp + 2) / 5 + 1;
  const int64_t m = mp < 10 ? mp + 3 : mp - 9;
  *year = (int)(yoe + era * 400 + (m <= 2 ? 1 : 0));
  *month = (int)m;
  *day = (int)d;
}

// The previous frame is still transmitting while writing is set; write_done clears it.
static void chip_timer_event(void *user_data) {
  chip_state_t *state = (chip_state_t *)user_data;
  if (state->writing) {
    return;
  }

  const double north_m = attr_read_float(state->north_attr);
  const double east_m = attr_read_float(state->east_attr);
  const double latitude = kBaseLatitude + north_m / kMetersPerDegreeLatitude;
  const double longitude = kBaseLongitude + east_m / (kMetersPerDegreeLatitude * kCosBaseLatitude);

  int year, month, day, hour, minute, second;
  utc_from_epoch(kStartEpochSeconds + get_sim_nanos() / 1000000000ULL, &year, &month, &day, &hour, &minute,
                 &second);

  int lat_deg, lon_deg;
  double lat_min, lon_min;
  split_degrees(latitude, &lat_deg, &lat_min);
  split_degrees(longitude, &lon_deg, &lon_min);
  const char lat_hemisphere = latitude < 0.0 ? 'S' : 'N';
  const char lon_hemisphere = longitude < 0.0 ? 'W' : 'E';

  char time_field[16];
  snprintf(time_field, sizeof(time_field), "%02d%02d%02d.00", hour, minute, second);

  // Speed and course stay zero: the simulated collar is static apart from the sliders.
  char rmc[kBodyCapacity];
  snprintf(rmc, sizeof(rmc), "GPRMC,%s,A,%02d%07.4f,%c,%03d%07.4f,%c,0.0,0.0,%02d%02d%02d,,,A", time_field,
           lat_deg, lat_min, lat_hemisphere, lon_deg, lon_min, lon_hemisphere, day, month, year % 100);

  // Fix quality 1 with 8 satellites and HDOP 0.9; altitude is not simulated, so it stays 0.0.
  char gga[kBodyCapacity];
  snprintf(gga, sizeof(gga), "GPGGA,%s,%02d%07.4f,%c,%03d%07.4f,%c,1,08,0.9,0.0,M,0.0,M,,", time_field, lat_deg,
           lat_min, lat_hemisphere, lon_deg, lon_min, lon_hemisphere);

  int used = append_sentence(state->frame, sizeof(state->frame), rmc);
  used += append_sentence(state->frame + used, sizeof(state->frame) - (size_t)used, gga);
  state->writing = uart_write(state->uart, (uint8_t *)state->frame, (uint32_t)used);
}

void chip_init(void) {
  chip.north_attr = attr_init_float("northMeters", 0.0f);
  chip.east_attr = attr_init_float("eastMeters", 0.0f);
  chip.writing = false;

  const uart_config_t uart_config = {
    .tx = pin_init("TX", INPUT_PULLUP),
    .rx = pin_init("RX", INPUT),
    .baud_rate = 9600,
    .write_done = on_uart_write_done,
    .user_data = &chip,
  };
  chip.uart = uart_init(&uart_config);

  timer_config_t timer_config = {
    .callback = chip_timer_event,
    .user_data = &chip,
  };
  timer_t timer = timer_init(&timer_config);
  timer_start(timer, 1000000, true);

  printf("NEO-6M chip initialized\n");
}
