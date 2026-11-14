#ifndef TELEMETRY_READING_H
#define TELEMETRY_READING_H

#include <stddef.h>
#include <stdint.h>

// Why a temperature has no value; each cause has its own reason for the edge.
enum class TemperatureFailure : uint8_t { None, NotResponding, OutOfRange };
enum class HeartRateFailure : uint8_t { None, NoContact, OutOfRange };

// Fixed-size fields let the buffer hold readings without heap allocation.
// Widest fields come first so padding stays out of the stored blob.
struct TelemetryReading {
    static constexpr size_t kEventIdCapacity = 48;
    static constexpr size_t kRecordedAtCapacity = 25;
    // Raise it when a field changes, so a queue stored with the old layout is discarded.
    static constexpr uint32_t kLayoutVersion = 3;

    double latitude = 0.0;
    double longitude = 0.0;
    float accuracyMeters = 0.0f;
    float bodyTemperature = 0.0f;
    float activityIndex = 0.0f;
    float batteryLevel = 0.0f;
    float heartRate = 0.0f;
    float methaneIndex = 0.0f;
    char eventId[kEventIdCapacity] = "";
    // Stays empty while the clock is not valid; the edge then stamps the reading itself.
    char recordedAt[kRecordedAtCapacity] = "";
    bool hasBodyTemperature = false;
    bool hasActivityIndex = false;
    bool hasBatteryLevel = false;
    bool hasPosition = false;
    bool hasHeartRate = false;
    bool hasMethaneIndex = false;
    // A failed sensor sends a reason in invalid_reasons instead of a value.
    TemperatureFailure bodyTemperatureFailure = TemperatureFailure::None;
    HeartRateFailure heartRateFailure = HeartRateFailure::None;
    bool activityIndexFailed = false;
    bool positionFailed = false;
    bool methaneIndexWarmingUp = false;

    bool bodyTemperatureFailed() const {
        return bodyTemperatureFailure != TemperatureFailure::None;
    }

    bool heartRateFailed() const {
        return heartRateFailure != HeartRateFailure::None;
    }

    bool hasMeasurement() const {
        return hasBodyTemperature || bodyTemperatureFailed() || hasActivityIndex || activityIndexFailed ||
               hasBatteryLevel || hasPosition || positionFailed || hasHeartRate || heartRateFailed() ||
               hasMethaneIndex || methaneIndexWarmingUp;
    }
};

// Four 32-byte NVS entries per slot keep 60 queued readings inside the default NVS partition.
static_assert(sizeof(TelemetryReading) <= 128, "TelemetryReading outgrew its NVS slot budget");

#endif  // TELEMETRY_READING_H
