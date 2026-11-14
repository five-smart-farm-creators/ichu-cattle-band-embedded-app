#ifndef ACTIVITY_INDEX_CALCULATOR_H
#define ACTIVITY_INDEX_CALCULATOR_H

#include <stdint.h>

// Accumulates acceleration samples over one reporting interval and reduces them to one index.
class ActivityIndexCalculator {
public:
    void addSample(float ax, float ay, float az);
    bool hasSamples() const;
    float index() const;
    void reset();

private:
    float deviationSum_ = 0.0f;
    uint32_t sampleCount_ = 0;
};

#endif  // ACTIVITY_INDEX_CALCULATOR_H
