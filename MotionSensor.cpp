#include "MotionSensor.h"

#include <Arduino.h>

bool MotionSensor::begin() {
    ready_ = mpu_.begin();
    if (!ready_) {
        Serial.println("[sensors] MPU-6050 not found on I2C");
        return false;
    }
    // +-4 g keeps finer resolution than +-8 g, which is enough for normal cattle movement.
    mpu_.setAccelerometerRange(MPU6050_RANGE_4_G);
    return true;
}

bool MotionSensor::readAcceleration(float& ax, float& ay, float& az) {
    // Without a sensor the zeros would read as 9.8 m/s^2 of deviation, so reads are refused.
    if (!ready_) {
        return false;
    }
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    if (!mpu_.getEvent(&accel, &gyro, &temp)) {
        return false;
    }
    ax = accel.acceleration.x;
    ay = accel.acceleration.y;
    az = accel.acceleration.z;
    return true;
}
