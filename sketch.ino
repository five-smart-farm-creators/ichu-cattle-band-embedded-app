// Composition root: wires the sensors, the clock and both routes into one non-blocking loop.
#include <Arduino.h>
#include <Wire.h>

#include "ActivityIndexCalculator.h"
#include "BandConnectivityService.h"
#include "BatteryMonitor.h"
#include "BleGatewayTransport.h"
#include "CollarClock.h"
#include "CollarConfig.h"
#include "EdgeHttpTransport.h"
#include "GasSensor.h"
#include "GpsReceiver.h"
#include "MotionSensor.h"
#include "PulseSensor.h"
#include "ReadingBuffer.h"
#include "TelemetryReading.h"
#include "TemperatureSensor.h"

static TemperatureSensor temperatureSensor;
static MotionSensor motionSensor;
static GpsReceiver gpsReceiver;
static BatteryMonitor batteryMonitor;
static PulseSensor pulseSensor;
static GasSensor gasSensor;
static CollarClock collarClock;
static ActivityIndexCalculator activity;
static ReadingBuffer buffer;
static EdgeHttpTransport edgeTransport;
static BleGatewayTransport bleTransport;
static BandConnectivityService connectivity(bleTransport, edgeTransport, buffer);
static uint32_t lastMotionSampleMs = 0;
static uint32_t lastPulseSampleMs = 0;
static uint32_t lastReadingMs = 0;

static void printSummary(const TelemetryReading& reading, size_t queued) {
    Serial.printf("[telemetry] event=%s", reading.eventId);
    if (reading.hasBodyTemperature) {
        Serial.printf(" temp=%.2fC", reading.bodyTemperature);
    } else if (reading.bodyTemperatureFailure == TemperatureFailure::OutOfRange) {
        Serial.print(" temp=out-of-range");
    } else if (reading.bodyTemperatureFailed()) {
        Serial.print(" temp=failed");
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
    } else if (reading.positionFailed) {
        Serial.print(" position=failed");
    }
    Serial.printf(" queued=%u\n", static_cast<unsigned>(queued));
}

static void queueReading() {
    TelemetryReading reading;
    collarClock.nextEventId(reading.eventId, sizeof(reading.eventId));
    if (collarClock.isValid()) {
        collarClock.formatUtc(reading.recordedAt, sizeof(reading.recordedAt));
    }
    float celsius = 0.0f;
    if (!temperatureSensor.read(celsius)) {
        reading.bodyTemperatureFailure = TemperatureFailure::NotResponding;
    } else if (celsius < CollarConfig::kMinBodyTemperatureC || celsius > CollarConfig::kMaxBodyTemperatureC) {
        // The edge rejects the whole reading for this value, so only the temperature is dropped.
        reading.bodyTemperatureFailure = TemperatureFailure::OutOfRange;
    } else {
        reading.hasBodyTemperature = true;
        reading.bodyTemperature = celsius;
    }
    if (activity.hasSamples()) {
        reading.hasActivityIndex = true;
        reading.activityIndex = activity.index();
    } else {
        // No samples means the MPU-6050 did not answer during the whole interval.
        reading.activityIndexFailed = true;
    }
    activity.reset();
    if (!pulseSensor.hasHeartRate()) {
        reading.heartRateFailure = HeartRateFailure::NoContact;
    } else {
        const float bpm = pulseSensor.heartRate();
        if (bpm < CollarConfig::kMinHeartRateBpm || bpm > CollarConfig::kMaxHeartRateBpm) {
            reading.heartRateFailure = HeartRateFailure::OutOfRange;
        } else {
            reading.hasHeartRate = true;
            reading.heartRate = bpm;
        }
    }
    pulseSensor.reset();
    if (gasSensor.isWarmingUp(millis())) {
        reading.methaneIndexWarmingUp = true;
    } else {
        reading.hasMethaneIndex = true;
        reading.methaneIndex = gasSensor.methaneIndex();
    }
    reading.hasBatteryLevel = true;
    reading.batteryLevel = batteryMonitor.levelPercent();
    reading.hasPosition = gpsReceiver.position(reading.latitude, reading.longitude, reading.accuracyMeters);
    // A GPS that answers without a fix leaves the position absent; that is not a failure.
    if (!gpsReceiver.isResponding()) {
        reading.positionFailed = true;
    }
    if (!reading.hasMeasurement()) {
        return;
    }
    if (buffer.push(reading)) {
        Serial.println("[buffer] full, dropped the oldest reading");
    }
    printSummary(reading, buffer.size());
}

void setup() {
    Serial.begin(115200);
    Wire.begin(CollarConfig::kI2cSdaPin, CollarConfig::kI2cSclPin);
    buffer.begin();
    if (buffer.size() > 0) {
        Serial.printf("[buffer] restored %u readings\n", static_cast<unsigned>(buffer.size()));
    }
    temperatureSensor.begin();
    motionSensor.begin();
    gpsReceiver.begin();
    gasSensor.begin(millis());
    collarClock.begin();
    edgeTransport.begin();
    if (CollarConfig::kBleEnabled) {
        bleTransport.begin();
    }
}

void loop() {
    const uint32_t now = millis();
    gpsReceiver.poll();
    edgeTransport.poll();
    collarClock.update(edgeTransport.isAvailable(), gpsReceiver);

    if (now - lastMotionSampleMs >= CollarConfig::kMotionSampleIntervalMs) {
        lastMotionSampleMs = now;
        float ax = 0.0f;
        float ay = 0.0f;
        float az = 0.0f;
        if (motionSensor.readAcceleration(ax, ay, az)) {
            activity.addSample(ax, ay, az);
        }
    }
    if (now - lastPulseSampleMs >= CollarConfig::kPulseSampleIntervalMs) {
        lastPulseSampleMs = now;
        pulseSensor.addSample(now);
    }
    if (now - lastReadingMs >= CollarConfig::kReadingIntervalMs) {
        lastReadingMs = now;
        queueReading();
    }
    connectivity.poll();
}
