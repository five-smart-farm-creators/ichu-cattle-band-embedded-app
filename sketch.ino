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
#include "GpsReceiver.h"
#include "MotionSensor.h"
#include "ReadingBuffer.h"
#include "TelemetryReading.h"
#include "TemperatureSensor.h"

static TemperatureSensor temperatureSensor;
static MotionSensor motionSensor;
static GpsReceiver gpsReceiver;
static BatteryMonitor batteryMonitor;
static CollarClock collarClock;
static ActivityIndexCalculator activity;
static ReadingBuffer buffer;
static EdgeHttpTransport edgeTransport;
static BleGatewayTransport bleTransport;
static BandConnectivityService connectivity(bleTransport, edgeTransport, buffer);
static uint32_t lastMotionSampleMs = 0;
static uint32_t lastReadingMs = 0;

static void printSummary(const TelemetryReading& reading, size_t queued) {
    Serial.printf("[telemetry] event=%s", reading.eventId);
    if (reading.hasBodyTemperature) {
        Serial.printf(" temp=%.2fC", reading.bodyTemperature);
    }
    if (reading.hasActivityIndex) {
        Serial.printf(" activity=%.2f", reading.activityIndex);
    }
    if (reading.hasBatteryLevel) {
        Serial.printf(" battery=%.2f%%", reading.batteryLevel);
    }
    if (reading.hasPosition) {
        Serial.printf(" position=%.6f,%.6f", reading.latitude, reading.longitude);
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
    if (temperatureSensor.read(celsius)) {
        reading.hasBodyTemperature = true;
        reading.bodyTemperature = celsius;
    }
    if (activity.hasSamples()) {
        reading.hasActivityIndex = true;
        reading.activityIndex = activity.index();
    }
    activity.reset();
    reading.hasBatteryLevel = true;
    reading.batteryLevel = batteryMonitor.levelPercent();
    reading.hasPosition = gpsReceiver.position(reading.latitude, reading.longitude, reading.accuracyMeters);
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
    temperatureSensor.begin();
    motionSensor.begin();
    gpsReceiver.begin();
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
    if (now - lastReadingMs >= CollarConfig::kReadingIntervalMs) {
        lastReadingMs = now;
        queueReading();
    }
    connectivity.poll();
}
