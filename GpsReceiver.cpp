#include "GpsReceiver.h"

#include <Arduino.h>

#include "CollarConfig.h"

void GpsReceiver::begin() {
    Serial1.begin(CollarConfig::kGpsBaud, SERIAL_8N1, CollarConfig::kGpsRxPin, CollarConfig::kGpsTxPin);
}

void GpsReceiver::poll() {
    while (Serial1.available() > 0) {
        gps_.encode(static_cast<char>(Serial1.read()));
    }
    if (!silenceReported_ && millis() >= CollarConfig::kGpsSilenceTimeoutMs && gps_.charsProcessed() == 0) {
        silenceReported_ = true;
        Serial.println("[gps] no NMEA data received, check wiring");
    }
}

bool GpsReceiver::position(double& latitude, double& longitude, float& accuracyMeters) {
    if (!gps_.location.isValid() || gps_.location.age() > CollarConfig::kGpsMaxAgeMs || !gps_.hdop.isValid()) {
        return false;
    }
    latitude = gps_.location.lat();
    longitude = gps_.location.lng();
    accuracyMeters = static_cast<float>(gps_.hdop.hdop() * CollarConfig::kGpsUereMeters);
    return true;
}

bool GpsReceiver::utcDateTime(tm& utc) {
    if (!gps_.date.isValid() || !gps_.time.isValid()) {
        return false;
    }
    utc.tm_year = gps_.date.year() - 1900;
    utc.tm_mon = gps_.date.month() - 1;
    utc.tm_mday = gps_.date.day();
    utc.tm_hour = gps_.time.hour();
    utc.tm_min = gps_.time.minute();
    utc.tm_sec = gps_.time.second();
    return true;
}
