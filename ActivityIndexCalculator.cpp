#include "ActivityIndexCalculator.h"

#include <algorithm>
#include <cmath>

#include "CollarConfig.h"

namespace {

constexpr float kStandardGravityMs2 = 9.80665f;

}  // namespace

void ActivityIndexCalculator::addSample(const float magnitudeInGForce) {
    const float deviationMs2 = (magnitudeInGForce - 1.0f) * kStandardGravityMs2;
    sumOfSquaredDeviations_ += deviationMs2 * deviationMs2;
    ++sampleCount_;
}

// Movement shows up as deviation from gravity, so a resting animal scores near zero.
float ActivityIndexCalculator::index() const {
    if (sampleCount_ == 0) {
        return 0.0f;
    }
    const float rmsDeviationMs2 = std::sqrt(sumOfSquaredDeviations_ / static_cast<float>(sampleCount_));
    return std::clamp(100.0f * rmsDeviationMs2 / CollarConfig::kActivityFullScale, 0.0f, 100.0f);
}

void ActivityIndexCalculator::reset() {
    sumOfSquaredDeviations_ = 0.0f;
    sampleCount_ = 0;
}
