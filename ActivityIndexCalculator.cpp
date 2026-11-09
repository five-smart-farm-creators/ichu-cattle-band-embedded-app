#include "ActivityIndexCalculator.h"

#include <algorithm>
#include <cmath>

#include "CollarConfig.h"

namespace {

constexpr float kStandardGravity = 9.80665f;

}  // namespace

void ActivityIndexCalculator::addSample(float ax, float ay, float az) {
    const float magnitude = std::sqrt(ax * ax + ay * ay + az * az);
    deviationSum_ += std::fabs(magnitude - kStandardGravity);
    ++sampleCount_;
}

bool ActivityIndexCalculator::hasSamples() const {
    return sampleCount_ > 0;
}

// Movement shows up as deviation from gravity, so a resting animal scores near zero.
float ActivityIndexCalculator::index() const {
    if (sampleCount_ == 0) {
        return 0.0f;
    }
    const float meanDeviation = deviationSum_ / static_cast<float>(sampleCount_);
    return std::clamp(100.0f * meanDeviation / CollarConfig::kActivityFullScale, 0.0f, 100.0f);
}

void ActivityIndexCalculator::reset() {
    deviationSum_ = 0.0f;
    sampleCount_ = 0;
}
