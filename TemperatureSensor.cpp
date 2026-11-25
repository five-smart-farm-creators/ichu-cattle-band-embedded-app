#include "TemperatureSensor.h"

#include <Arduino.h>

#include "CollarConfig.h"

namespace {

// 10 bits resolve 0.25 C in about 190 ms, a good trade-off for one reading every 10 s.
constexpr uint8_t kResolutionBits = 10;

}  // namespace

TemperatureSensor::TemperatureSensor() : oneWire_(CollarConfig::kTemperaturePin), sensors_(&oneWire_) {}

bool TemperatureSensor::begin() {
    sensors_.begin();
    if (sensors_.getDeviceCount() == 0) {
        Serial.println("[sensors] DS18B20 not found on the 1-Wire bus");
        return false;
    }
    sensors_.setResolution(kResolutionBits);
    return true;
}

bool TemperatureSensor::read(float& celsius) {
    sensors_.requestTemperatures();
    celsius = sensors_.getTempCByIndex(0);
    return celsius != DEVICE_DISCONNECTED_C;
}
