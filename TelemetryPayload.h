#ifndef TELEMETRY_PAYLOAD_H
#define TELEMETRY_PAYLOAD_H

#include <stddef.h>

#include "TelemetryReading.h"

// JSON body of POST /api/v1/monitoring/telemetry-readings, shared by both transports.
class TelemetryPayload {
public:
    // Typical bodies are about 350 bytes; the worst case is about 430; 512 is also the largest BLE attribute value.
    static constexpr size_t kMaxLength = 512;

    // Returns the body length, or 0 when the body does not fit in capacity.
    static size_t serialize(const TelemetryReading& reading, char* out, size_t capacity);
};

#endif  // TELEMETRY_PAYLOAD_H
