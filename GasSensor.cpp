#include "GasSensor.h"

#include <Arduino.h>
#include <algorithm>

#include "CollarConfig.h"

void GasSensor::begin(uint32_t nowMs) {
    poweredAtMs_ = nowMs;
}

bool GasSensor::isWarmingUp(uint32_t nowMs) const {
    return nowMs - poweredAtMs_ < CollarConfig::kGasWarmupMs;
}

float GasSensor::methaneIndex() const {
    const float mv = analogReadMilliVolts(CollarConfig::kGasPin);
    return std::clamp(100.0f * mv / CollarConfig::kGasFullScaleMv, 0.0f, 100.0f);
}
