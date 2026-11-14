#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include <stddef.h>
#include <stdint.h>

// SP-203 photoelectric pulse sensor on an ADC pin; beats are detected on the rising edge
// of the PPG wave and turned into beats per minute once per reading interval.
class PulseSensor {
public:
    void addSample(uint32_t nowMs);
    bool hasHeartRate() const;
    // Median beat interval, so one missed or doubled beat does not skew the rate.
    float heartRate() const;
    void reset();

private:
    static constexpr size_t kMaxIntervals = 64;

    uint16_t intervalsMs_[kMaxIntervals] = {};
    size_t intervalCount_ = 0;
    float peakMv_ = 0.0f;
    float troughMv_ = 0.0f;
    bool envelopeStarted_ = false;
    bool aboveThreshold_ = false;
    uint32_t lastBeatMs_ = 0;
};

#endif  // PULSE_SENSOR_H
