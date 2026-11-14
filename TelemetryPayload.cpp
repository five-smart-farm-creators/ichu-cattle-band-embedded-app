#include "TelemetryPayload.h"

#include <ArduinoJson.h>
#include <math.h>

#include "CollarConfig.h"

namespace {

// Rounding keeps the body compact at the precision the sensors can deliver.
double roundTo(double value, double scale) {
    return round(value * scale) / scale;
}

}  // namespace

size_t TelemetryPayload::serialize(const TelemetryReading& reading, char* out, size_t capacity) {
    JsonDocument document;
    document["device_id"] = CollarConfig::kDeviceId;
    document["event_id"] = reading.eventId;
    if (reading.recordedAt[0] != '\0') {
        document["recorded_at"] = reading.recordedAt;
    }
    if (reading.hasBodyTemperature) {
        document["body_temperature"] = roundTo(reading.bodyTemperature, 100.0);
    }
    if (reading.hasActivityIndex) {
        document["activity_index"] = roundTo(reading.activityIndex, 100.0);
    }
    if (reading.hasBatteryLevel) {
        document["battery_level"] = roundTo(reading.batteryLevel, 100.0);
    }
    if (reading.hasHeartRate) {
        document["heart_rate"] = roundTo(reading.heartRate, 100.0);
    }
    if (reading.hasMethaneIndex) {
        document["methane_index"] = roundTo(reading.methaneIndex, 100.0);
    }
    if (reading.hasPosition) {
        JsonObject position = document["position"].to<JsonObject>();
        position["latitude"] = roundTo(reading.latitude, 1e6);
        position["longitude"] = roundTo(reading.longitude, 1e6);
        position["accuracy_meters"] = roundTo(reading.accuracyMeters, 100.0);
    }
    if (reading.bodyTemperatureFailed() || reading.activityIndexFailed || reading.positionFailed ||
        reading.heartRateFailed() || reading.methaneIndexWarmingUp) {
        JsonObject invalidReasons = document["invalid_reasons"].to<JsonObject>();
        if (reading.bodyTemperatureFailure == TemperatureFailure::NotResponding) {
            invalidReasons["body_temperature"] = "Temperature sensor not responding";
        } else if (reading.bodyTemperatureFailure == TemperatureFailure::OutOfRange) {
            invalidReasons["body_temperature"] = "Temperature out of physiological range";
        }
        if (reading.activityIndexFailed) {
            invalidReasons["activity_index"] = "Motion sensor not responding";
        }
        if (reading.positionFailed) {
            invalidReasons["position"] = "GPS receiver not responding";
        }
        if (reading.heartRateFailure == HeartRateFailure::NoContact) {
            invalidReasons["heart_rate"] = "Pulse sensor without contact";
        } else if (reading.heartRateFailure == HeartRateFailure::OutOfRange) {
            invalidReasons["heart_rate"] = "Heart rate out of physiological range";
        }
        if (reading.methaneIndexWarmingUp) {
            invalidReasons["methane_index"] = "Gas sensor warming up";
        }
    }
    const size_t length = measureJson(document);
    if (length >= capacity) {
        return 0;
    }
    serializeJson(document, out, capacity);
    return length;
}
