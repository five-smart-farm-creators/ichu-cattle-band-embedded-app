#ifndef HEART_RATE_ESTIMATOR_H
#define HEART_RATE_ESTIMATOR_H

#include <stddef.h>
#include <stdint.h>

// SP-203 beat detection and heart rate. Named apart from the framework's PulseSensor, which drops
// rates outside 30-200 bpm before the device sees them and cannot report missing skin contact.
class HeartRateEstimator {
public:
    // Samples the pulse pin; the device calls it every kPulseSampleIntervalMs.
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

#endif  // HEART_RATE_ESTIMATOR_H
