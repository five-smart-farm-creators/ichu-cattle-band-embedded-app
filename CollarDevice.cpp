#include "CollarDevice.h"

#include <Arduino.h>
#include <algorithm>
#include <memory>

#include "CollarConfig.h"
#include "TelemetryPayload.h"
#include "TelemetryReadingPackage.h"

namespace {

// One line per reading; the Wokwi scenarios match substrings of these fields.
void printSummary(const TelemetryReading& reading) {
    Serial.printf("[telemetry] event=%s", reading.eventId);
    if (reading.hasBodyTemperature) {
        Serial.printf(" temp=%.2fC", reading.bodyTemperature);
    } else if (reading.bodyTemperatureOutOfRange) {
        Serial.print(" temp=out-of-range");
    }
    if (reading.hasActivityIndex) {
        Serial.printf(" activity=%.2f", reading.activityIndex);
    } else if (reading.activityIndexFailed) {
        Serial.print(" activity=failed");
    }
    if (reading.hasBatteryLevel) {
        Serial.printf(" battery=%.2f%%", reading.batteryLevel);
    }
    if (reading.hasHeartRate) {
        Serial.printf(" heart=%.2fbpm", reading.heartRate);
    } else if (reading.heartRateFailure == HeartRateFailure::OutOfRange) {
        Serial.print(" heart=out-of-range");
    } else if (reading.heartRateFailure == HeartRateFailure::NoContact) {
        Serial.print(" heart=no-contact");
    }
    if (reading.hasMethaneIndex) {
        Serial.printf(" methane=%.2f", reading.methaneIndex);
    } else if (reading.methaneIndexWarmingUp) {
        Serial.print(" methane=warming-up");
    }
    if (reading.hasPosition) {
        Serial.printf(" position=%.6f,%.6f", reading.latitude, reading.longitude);
    }
    Serial.println();
}

}  // namespace

CollarDevice::CollarDevice(WiFiConnectivityDriver& wifi, HttpGatewayClient& gateway)
    : Device(CollarConfig::kSchedulerTickMs),
      wifi_(wifi),
      gateway_(gateway),
      temperature_(CollarConfig::kTemperaturePin),
      // Only the motion sensor and the GPS report to this device; the other sensors are read at report time.
      motion_(CollarConfig::kI2cSdaPin, ImuSensor::DEFAULT_I2C_ADDRESS, ImuSensor::IMU_ACCEL_FS_4G,
              ImuSensor::IMU_GYRO_FS_250, this),
      gps_(Serial1, CollarConfig::kGpsRxPin, CollarConfig::kGpsTxPin, CollarConfig::kGpsBaud, this),
      gas_(CollarConfig::kGasPin) {
    clock_.begin();
    if (!motion_.initialize()) {
        MODEST_LOG_W("Collar", "MPU-6050 did not answer at startup");
    }
    motion_.setFaultThreshold(CollarConfig::kMotionFaultReads);

    // Registration must precede initializeAsynchronousEngine(), which rejects later calls.
    // The temperature is registered first, so each reading follows a fresh temperature sample.
    appendSensorToScheduler(&temperature_, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                            CollarConfig::kTemperatureSampleIntervalMs, 0);
    appendSensorToScheduler(this, kReadingEvent, CollarConfig::kReadingIntervalMs, 0);
    appendSensorToScheduler(&motion_, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                            CollarConfig::kMotionSampleIntervalMs, 0);
    appendSensorToScheduler(&gps_, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                            CollarConfig::kGpsSampleIntervalMs, 0);
    appendSensorToScheduler(&gas_, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                            CollarConfig::kGasSampleIntervalMs, 0);
    appendSensorToScheduler(this, kPulseSampleEvent, CollarConfig::kPulseSampleIntervalMs, 0);
    initializeAsynchronousEngine(CollarConfig::kTelemetryQueueLength);
}

void CollarDevice::on(Event event) {
    if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
        // The GPS tags its events with its RX pin; the motion sensor leaves its events untagged.
        if (event.sourceId == CollarConfig::kGpsRxPin) {
            gpsUpdatedAtMs_ = millis();
        } else {
            motionFailed_ = false;
            activity_.addSample(motion_.getAccelerationMagnitudeInGForce());
        }
    } else if (event.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER) {
        // Only the motion sensor reports read faults; the flag clears with its next changed sample.
        motionFailed_ = true;
    } else if (event.identifier == kPulseSampleEvent) {
        heart_.addSample(millis());
    } else if (event.identifier == kReadingEvent && millis() >= CollarConfig::kReadingIntervalMs) {
        // The first report fires right after boot, before any interval has filled.
        buildAndQueueReading();
    }
}

void CollarDevice::buildAndQueueReading() {
    clock_.setFromGpsWhenOffline(wifi_.isConnected(), gps_.getLocationData());
    TelemetryReading reading;
    clock_.nextEventId(reading.eventId, sizeof(reading.eventId));
    if (clock_.isValid()) {
        clock_.formatUtc(reading.recordedAt, sizeof(reading.recordedAt));
    }

    const float celsius = temperature_.getTemperatureInCelsius();
    // The edge rejects the whole reading for this value, so only the temperature is dropped.
    if (celsius < CollarConfig::kMinBodyTemperatureC || celsius > CollarConfig::kMaxBodyTemperatureC) {
        reading.bodyTemperatureOutOfRange = true;
    } else {
        reading.hasBodyTemperature = true;
        reading.bodyTemperature = celsius;
    }

    if (motionFailed_) {
        reading.activityIndexFailed = true;
    } else {
        reading.hasActivityIndex = true;
        reading.activityIndex = activity_.index();
    }
    activity_.reset();

    if (!heart_.hasHeartRate()) {
        reading.heartRateFailure = HeartRateFailure::NoContact;
    } else {
        const float bpm = heart_.heartRate();
        if (bpm < CollarConfig::kMinHeartRateBpm || bpm > CollarConfig::kMaxHeartRateBpm) {
            reading.heartRateFailure = HeartRateFailure::OutOfRange;
        } else {
            reading.hasHeartRate = true;
            reading.heartRate = bpm;
        }
    }
    heart_.reset();

    if (millis() < CollarConfig::kGasWarmupMs) {
        reading.methaneIndexWarmingUp = true;
    } else {
        const float rawCounts = static_cast<float>(gas_.getRawAnalogValue());
        reading.hasMethaneIndex = true;
        reading.methaneIndex = std::clamp(100.0f * rawCounts / static_cast<float>(ADC_MAX_VALUE), 0.0f, 100.0f);
    }

    reading.hasBatteryLevel = true;
    reading.batteryLevel = PowerManager::getBatteryLevelInPercentage();

    // Stale data or a sentence without a fix must not place the animal, so both are checked.
    const LocationData& location = gps_.getLocationData();
    const bool positionFresh = millis() - gpsUpdatedAtMs_ <= CollarConfig::kGpsMaxAgeMs;
    if (positionFresh && location.fixQuality > 0 && location.isDataValid &&
        location.horizontalDilutionOfPrecision > 0.0f) {
        reading.hasPosition = true;
        reading.latitude = location.latitudeInDegrees;
        reading.longitude = location.longitudeInDegrees;
        reading.accuracyMeters = location.horizontalDilutionOfPrecision * CollarConfig::kGpsUereMeters;
    }

    enqueueTelemetryPayload(std::make_unique<TelemetryReadingPackage>(reading));
    printSummary(reading);
}

SendOutcome CollarDevice::post(const char* body) const {
    if (gateway_.sendRequest(HttpGatewayClient::HttpMethod::Post, body)) {
        return SendOutcome::Delivered;
    }
    const int status = gateway_.getLastResponseStatusCode();
    // The edge answers an identical body the same way every time, so resending cannot help.
    if (status == 400 || status == 409) {
        return SendOutcome::Rejected;
    }
    // Timeouts (negative codes), 401 and 5xx may clear up, so the body stays queued.
    return SendOutcome::Retry;
}

void CollarDevice::processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const {
    // The device enqueues only TelemetryReadingPackage; the framework's diagnostics package is never requested.
    const TelemetryReading& reading = static_cast<const TelemetryReadingPackage*>(rawQueueItemPayload)->reading();
    char body[TelemetryPayload::kMaxLength];
    if (TelemetryPayload::serialize(reading, body, sizeof(body)) == 0) {
        Serial.printf("[edge] %s exceeds the payload buffer, dropped\n", reading.eventId);
        return;
    }
    // sendRequest() would report a stale status code without Wi-Fi, so the link is checked first.
    if (!wifi_.canTransmit()) {
        wifi_.connect();
        TelemetryBuffer::store(String(body));
        return;
    }
    const SendOutcome outcome = post(body);
    const int status = gateway_.getLastResponseStatusCode();
    switch (outcome) {
        case SendOutcome::Delivered:
            Serial.printf("[edge] delivered %s (%d)\n", reading.eventId, status);
            replayBufferedBodies();
            break;
        case SendOutcome::Rejected:
            Serial.printf("[edge] rejected %s (%d), resending the same body cannot succeed\n", reading.eventId, status);
            break;
        case SendOutcome::Retry:
            Serial.printf("[edge] retry %s (%d)\n", reading.eventId, status);
            TelemetryBuffer::store(String(body));
            break;
    }
}

void CollarDevice::replayBufferedBodies() const {
    String stored;
    int sent = 0;
    while (wifi_.canTransmit() && TelemetryBuffer::retrieveNext(stored)) {
        if (post(stored.c_str()) == SendOutcome::Retry) {
            // Stopping at the first failure keeps an unreachable edge from being hammered.
            TelemetryBuffer::store(stored);
            break;
        }
        ++sent;
    }
    if (sent > 0) {
        Serial.printf("[buffer] sent %d buffered readings\n", sent);
    }
}

void CollarDevice::maintainNetworkServices() const {
    const bool connected = wifi_.isConnected();
    if (connected != wasConnected_) {
        wasConnected_ = connected;
        if (connected) {
            Serial.printf("[wifi] connected, IP %s\n", wifi_.getLocalIpAddress().toString().c_str());
        } else {
            Serial.println("[wifi] disconnected");
        }
    }
    if (connected && !ntpStarted_) {
        // Started once: the call waits up to 5 s for the first sync, and SNTP keeps the clock current afterwards.
        ntpStarted_ = true;
        Serial.println("[clock] NTP sync started");
        TimeSynchronizationDriver::initialize(CollarConfig::kTimeZone, CollarConfig::kNtpServer);
    }
}
