#ifndef TELEMETRY_READING_H
#define TELEMETRY_READING_H

#include <stddef.h>

// Fixed-size fields let the buffer hold readings without heap allocation.
struct TelemetryReading {
    static constexpr size_t kEventIdCapacity = 48;
    static constexpr size_t kRecordedAtCapacity = 25;

    char eventId[kEventIdCapacity] = "";
    // Stays empty while the clock is not valid; the edge then stamps the reading itself.
    char recordedAt[kRecordedAtCapacity] = "";
    bool hasBodyTemperature = false;
    float bodyTemperature = 0.0f;
    bool hasActivityIndex = false;
    float activityIndex = 0.0f;
    bool hasBatteryLevel = false;
    float batteryLevel = 0.0f;
    bool hasPosition = false;
    double latitude = 0.0;
    double longitude = 0.0;
    float accuracyMeters = 0.0f;

    bool hasMeasurement() const {
        return hasBodyTemperature || hasActivityIndex || hasBatteryLevel || hasPosition;
    }
};

#endif  // TELEMETRY_READING_H
