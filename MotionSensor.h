#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <Adafruit_MPU6050.h>

// MPU-6050 accelerometer; the gyroscope is read only because the library returns it together.
class MotionSensor {
public:
    bool begin();
    bool readAcceleration(float& ax, float& ay, float& az);

private:
    Adafruit_MPU6050 mpu_;
    bool ready_ = false;
};

#endif  // MOTION_SENSOR_H
