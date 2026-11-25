#ifndef COLLAR_CLOCK_H
#define COLLAR_CLOCK_H

#include <stddef.h>
#include <stdint.h>

#include "LocationData.h"

// Event ids that stay unique across reboots, and UTC time from the system clock that NTP or GPS sets.
// TimeSynchronizationDriver::isSynchronized() ignores GPS time and stays false after a late NTP sync.
class CollarClock {
public:
    // Counts this boot; call once before nextEventId().
    void begin();
    // Sets the clock from GPS time, but only while Wi-Fi is down and NTP cannot run.
    void setFromGpsWhenOffline(bool wifiConnected, const LocationData& location);
    bool isValid() const;
    void formatUtc(char* out, size_t capacity) const;
    void nextEventId(char* out, size_t capacity);

private:
    uint32_t bootCount_ = 0;
    uint32_t sequence_ = 0;
};

#endif  // COLLAR_CLOCK_H
