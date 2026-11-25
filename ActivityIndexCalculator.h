#ifndef ACTIVITY_INDEX_CALCULATOR_H
#define ACTIVITY_INDEX_CALCULATOR_H

#include <stdint.h>

// Reduces the acceleration magnitudes of one reporting interval to one index.
class ActivityIndexCalculator {
public:
    // Magnitude of the acceleration vector in g, as the framework's ImuSensor reports it.
    void addSample(float magnitudeInGForce);
    // Zero when no sample arrived: the ImuSensor reports only changed readings, so a quiet animal has none.
    float index() const;
    void reset();

private:
    float sumOfSquaredDeviations_ = 0.0f; ///< Squared deviation from 1 g, in (m/s^2)^2.
    uint32_t sampleCount_ = 0;
};

#endif  // ACTIVITY_INDEX_CALCULATOR_H
