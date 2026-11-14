#include "CollarClock.h"

#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "CollarConfig.h"

namespace {

// 2024-01-01T00:00:00Z: an earlier clock has never been set by NTP or GPS.
constexpr time_t kEarliestValidEpoch = 1704067200;
constexpr char kPreferencesNamespace[] = "ichu";
constexpr char kBootCounterKey[] = "boot";

// Days since 1970-01-01 for a proleptic Gregorian date; mktime would depend on the local TZ.
int64_t daysFromCivil(int64_t year, int64_t month, int64_t day) {
    year -= month <= 2 ? 1 : 0;
    const int64_t era = (year >= 0 ? year : year - 399) / 400;
    const int64_t yearOfEra = year - era * 400;
    const int64_t dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const int64_t dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

// Returns false when the GPS date is earlier than the valid-clock threshold.
bool setClockFromGps(const tm& utc) {
    const int64_t days = daysFromCivil(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday);
    const int64_t seconds = days * 86400 + utc.tm_hour * 3600 + utc.tm_min * 60 + utc.tm_sec;
    if (seconds < kEarliestValidEpoch) {
        return false;
    }
    timeval now{};
    now.tv_sec = static_cast<time_t>(seconds);
    settimeofday(&now, nullptr);
    return true;
}

}  // namespace

void CollarClock::begin() {
    Preferences preferences;
    preferences.begin(kPreferencesNamespace, false);
    bootCount_ = preferences.getUInt(kBootCounterKey, 0) + 1;
    preferences.putUInt(kBootCounterKey, bootCount_);
    preferences.end();
}

void CollarClock::update(bool wifiConnected, GpsReceiver& gps) {
    if (wifiConnected && !ntpRequested_) {
        configTime(0, 0, CollarConfig::kNtpServer);
        ntpRequested_ = true;
        Serial.println("[clock] NTP sync started");
    }
    // NTP has priority; GPS time is used only while there is no Wi-Fi to reach an NTP server.
    tm utc{};
    if (!wifiConnected && !isValid() && gps.utcDateTime(utc) && setClockFromGps(utc)) {
        Serial.println("[clock] set from GPS");
    }
}

bool CollarClock::isValid() const {
    return time(nullptr) >= kEarliestValidEpoch;
}

void CollarClock::formatUtc(char* out, size_t capacity) const {
    const time_t now = time(nullptr);
    tm utc{};
    gmtime_r(&now, &utc);
    strftime(out, capacity, "%Y-%m-%dT%H:%M:%SZ", &utc);
}

// The boot counter keeps ids unique across reboots; the sequence keeps a retry identical.
void CollarClock::nextEventId(char* out, size_t capacity) {
    ++sequence_;
    snprintf(out, capacity, "%s-%06lu-%06lu", CollarConfig::kDeviceSerial,
             static_cast<unsigned long>(bootCount_), static_cast<unsigned long>(sequence_));
}
