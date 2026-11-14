#ifndef TELEMETRY_READING_H
#define TELEMETRY_READING_H

#include <stddef.h>
#include <stdint.h>

// Why a temperature has no value; each cause has its own reason for the edge.
enum class TemperatureFailure : uint8_t { None, NotResponding, OutOfRange };

// Fixed-size fields let the buffer hold readings without heap allocation.
struct TelemetryReading {
    static constexpr size_t kEventIdCapacity = 48;
    static constexpr size_t kRecordedAtCapacity = 25;
    // Raise it when a field changes, so a queue stored with the old layout is discarded.
    static constexpr uint32_t kLayoutVersion = 2;

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
    // A failed sensor sends a reason in invalid_reasons instead of a value.
    TemperatureFailure bodyTemperatureFailure = TemperatureFailure::None;
    bool activityIndexFailed = false;
    bool positionFailed = false;

    bool bodyTemperatureFailed() const {
        return bodyTemperatureFailure != TemperatureFailure::None;
    }

    bool hasMeasurement() const {
        return hasBodyTemperature || bodyTemperatureFailed() || hasActivityIndex || activityIndexFailed ||
               hasBatteryLevel || hasPosition || positionFailed;
    }
};

#endif  // TELEMETRY_READING_H
