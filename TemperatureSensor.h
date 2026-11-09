#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <DallasTemperature.h>
#include <OneWire.h>

// DS18B20 on a 1-Wire bus; each read blocks for one conversion, about 190 ms at 10 bits.
class TemperatureSensor {
public:
    TemperatureSensor();
    bool begin();
    bool read(float& celsius);

private:
    OneWire oneWire_;
    DallasTemperature sensors_;
};

#endif  // TEMPERATURE_SENSOR_H
