#ifndef BAND_CONNECTIVITY_SERVICE_H
#define BAND_CONNECTIVITY_SERVICE_H

#include <stdint.h>

#include "BleGatewayTransport.h"
#include "EdgeHttpTransport.h"
#include "ReadingBuffer.h"
#include "SendOutcome.h"
#include "TelemetryReading.h"

// Sends the oldest buffered reading over BLE when a gateway is ready, otherwise over Wi-Fi HTTP.
class BandConnectivityService {
public:
    BandConnectivityService(BleGatewayTransport& ble, EdgeHttpTransport& edge, ReadingBuffer& buffer);
    void poll();

private:
    void complete(const char* eventId, SendOutcome outcome);

    BleGatewayTransport& ble_;
    EdgeHttpTransport& edge_;
    ReadingBuffer& buffer_;
    bool bleInFlight_ = false;
    // A full buffer may drop the reading in flight, so completion checks the id before popping.
    char inFlightEventId_[TelemetryReading::kEventIdCapacity] = "";
    bool backingOff_ = false;
    uint32_t failedAtMs_ = 0;
};

#endif  // BAND_CONNECTIVITY_SERVICE_H
