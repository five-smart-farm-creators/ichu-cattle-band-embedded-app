#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

// Li-ion cell behind a 1:2 divider on an ADC pin, reported as a percentage.
class BatteryMonitor {
public:
    float levelPercent() const;
};

#endif  // BATTERY_MONITOR_H
