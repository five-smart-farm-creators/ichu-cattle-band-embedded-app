#ifndef READING_BUFFER_H
#define READING_BUFFER_H

#include <stddef.h>

#include "CollarConfig.h"
#include "TelemetryReading.h"

// Keeps readings in RAM until a transport confirms them; RAM is lost on power loss.
class ReadingBuffer {
public:
    // Returns true when the oldest reading was dropped to make room.
    bool push(const TelemetryReading& reading);
    // Callers check empty() first.
    const TelemetryReading& front() const;
    void pop();
    size_t size() const;
    bool empty() const;

private:
    static constexpr size_t kCapacity = CollarConfig::kBufferCapacity;

    TelemetryReading readings_[kCapacity];
    size_t head_ = 0;
    size_t count_ = 0;
};

#endif  // READING_BUFFER_H
