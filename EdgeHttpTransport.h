#ifndef EDGE_HTTP_TRANSPORT_H
#define EDGE_HTTP_TRANSPORT_H

#include "SendOutcome.h"
#include "TelemetryReading.h"

// Wi-Fi station and HTTP POST to the Edge API; send() blocks for at most kHttpTimeoutMs.
class EdgeHttpTransport {
public:
    void begin();
    void poll();
    bool isAvailable() const;
    SendOutcome send(const TelemetryReading& reading);

private:
    bool wasConnected_ = false;
};

#endif  // EDGE_HTTP_TRANSPORT_H
