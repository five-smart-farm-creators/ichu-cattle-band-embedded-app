#ifndef READING_BUFFER_H
#define READING_BUFFER_H

#include <Preferences.h>
#include <stddef.h>

#include "CollarConfig.h"
#include "TelemetryReading.h"

// Readings wait here until a transport confirms them; NVS mirrors them so they survive
// reboots and power loss. One write every 10 s is acceptable because NVS levels the wear.
class ReadingBuffer {
public:
    // Restores the queue saved by the previous run; call once before push().
    void begin();
    // Returns true when the oldest reading was dropped to make room.
    bool push(const TelemetryReading& reading);
    // Callers check empty() first.
    const TelemetryReading& front() const;
    void pop();
    size_t size() const;
    bool empty() const;

private:
    static constexpr size_t kCapacity = CollarConfig::kBufferCapacity;

    bool restore();
    void saveCounters();

    Preferences preferences_;
    TelemetryReading readings_[kCapacity];
    size_t head_ = 0;
    size_t count_ = 0;
};

#endif  // READING_BUFFER_H
