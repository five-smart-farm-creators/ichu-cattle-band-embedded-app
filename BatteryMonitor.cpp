#include "BatteryMonitor.h"

#include <Arduino.h>
#include <algorithm>

#include "CollarConfig.h"

float BatteryMonitor::levelPercent() const {
    const float cellVolts =
        analogReadMilliVolts(CollarConfig::kBatteryPin) / 1000.0f * CollarConfig::kBatteryDividerRatio;
    const float span = CollarConfig::kBatteryFullVolts - CollarConfig::kBatteryEmptyVolts;
    const float fraction = (cellVolts - CollarConfig::kBatteryEmptyVolts) / span;
    return std::clamp(100.0f * fraction, 0.0f, 100.0f);
}
