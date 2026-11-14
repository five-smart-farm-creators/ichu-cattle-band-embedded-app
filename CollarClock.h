#ifndef COLLAR_CLOCK_H
#define COLLAR_CLOCK_H

#include <stddef.h>
#include <stdint.h>

#include "GpsReceiver.h"

// UTC time from NTP or GPS, and event ids that stay unique across reboots.
class CollarClock {
public:
    void begin();
    void update(bool wifiConnected, GpsReceiver& gps);
    bool isValid() const;
    void formatUtc(char* out, size_t capacity) const;
    void nextEventId(char* out, size_t capacity);

private:
    bool ntpRequested_ = false;
    uint32_t bootCount_ = 0;
    uint32_t sequence_ = 0;
};

#endif  // COLLAR_CLOCK_H
