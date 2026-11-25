#ifndef TELEMETRY_READING_H
#define TELEMETRY_READING_H

#include <stddef.h>
#include <stdint.h>

// Why a heart rate has no value; each cause has its own reason for the edge.
enum class HeartRateFailure : uint8_t { None, NoContact, OutOfRange };

// Fixed-size char buffers keep the reading copyable without heap allocation of its own.
// Widest fields come first so padding stays out of the struct.
struct TelemetryReading {
    static constexpr size_t kEventIdCapacity = 48;
    static constexpr size_t kRecordedAtCapacity = 25;

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
    bool bodyTemperatureOutOfRange = false;
    bool bodyTemperatureFailed = false;
    HeartRateFailure heartRateFailure = HeartRateFailure::None;
    bool activityIndexFailed = false;
    bool methaneIndexWarmingUp = false;

    bool heartRateFailed() const {
        return heartRateFailure != HeartRateFailure::None;
    }
};

#endif  // TELEMETRY_READING_H
