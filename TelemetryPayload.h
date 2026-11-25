#ifndef TELEMETRY_PAYLOAD_H
#define TELEMETRY_PAYLOAD_H

#include <stddef.h>

#include "TelemetryReading.h"

// JSON body of POST /api/v1/monitoring/telemetry-readings. The framework's flat TelemetryWriter
// cannot express the nested position and invalid_reasons objects, so the body is built here.
class TelemetryPayload {
public:
    // Typical bodies are about 350 bytes; the worst case is about 430.
    static constexpr size_t kMaxLength = 512;

    // Returns the body length, or 0 when the body does not fit in capacity.
    static size_t serialize(const TelemetryReading& reading, char* out, size_t capacity);
};

#endif  // TELEMETRY_PAYLOAD_H
