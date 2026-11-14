#include "PulseSensor.h"

#include <Arduino.h>
#include <algorithm>

#include "CollarConfig.h"

void PulseSensor::addSample(uint32_t nowMs) {
    const float mv = analogReadMilliVolts(CollarConfig::kPulsePin);
    if (!envelopeStarted_) {
        peakMv_ = mv;
        troughMv_ = mv;
        envelopeStarted_ = true;
    }
    // The envelope decays toward the signal so the threshold follows changes in skin contact.
    peakMv_ = std::max(mv, peakMv_ - CollarConfig::kPulseEnvelopeDecayMv);
    troughMv_ = std::min(mv, troughMv_ + CollarConfig::kPulseEnvelopeDecayMv);
    const float amplitude = peakMv_ - troughMv_;
    if (amplitude < CollarConfig::kPulseMinAmplitudeMv) {
        // A flat signal means no skin contact; no beat can be trusted.
        aboveThreshold_ = false;
        lastBeatMs_ = 0;
        return;
    }
    const float threshold = troughMv_ + amplitude / 2.0f;
    if (!aboveThreshold_ && mv > threshold) {
        aboveThreshold_ = true;
        const uint32_t interval = nowMs - lastBeatMs_;
        if (lastBeatMs_ != 0 && interval < CollarConfig::kPulseRefractoryMs) {
            return;
        }
        // A longer gap is a lost signal or a blocked loop, not a real beat interval.
        if (lastBeatMs_ != 0 && interval <= CollarConfig::kPulseMaxIntervalMs && intervalCount_ < kMaxIntervals) {
            intervalsMs_[intervalCount_++] = static_cast<uint16_t>(interval);
        }
        lastBeatMs_ = nowMs;
    } else if (aboveThreshold_ && mv < threshold - CollarConfig::kPulseHysteresisMv) {
        aboveThreshold_ = false;
    }
}

bool PulseSensor::hasHeartRate() const {
    return intervalCount_ >= CollarConfig::kPulseMinIntervals;
}

float PulseSensor::heartRate() const {
    uint16_t sorted[kMaxIntervals];
    std::copy(intervalsMs_, intervalsMs_ + intervalCount_, sorted);
    std::sort(sorted, sorted + intervalCount_);
    const size_t middle = intervalCount_ / 2;
    const float medianMs = (intervalCount_ % 2 == 1) ? sorted[middle] : (sorted[middle - 1] + sorted[middle]) / 2.0f;
    return 60000.0f / medianMs;
}

void PulseSensor::reset() {
    intervalCount_ = 0;
}
