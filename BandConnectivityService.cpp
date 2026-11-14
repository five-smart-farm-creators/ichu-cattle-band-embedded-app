#include "BandConnectivityService.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "CollarConfig.h"

BandConnectivityService::BandConnectivityService(BleGatewayTransport& ble, EdgeHttpTransport& edge,
                                                 ReadingBuffer& buffer)
    : ble_(ble), edge_(edge), buffer_(buffer) {}

void BandConnectivityService::poll() {
    if (bleInFlight_) {
        SendOutcome outcome = SendOutcome::Retry;
        if (ble_.pollOutcome(outcome)) {
            bleInFlight_ = false;
            complete(inFlightEventId_, outcome);
        }
        return;
    }
    // Subtraction keeps the back-off correct when millis() wraps around.
    if (backingOff_ && millis() - failedAtMs_ < CollarConfig::kRetryBackoffMs) {
        return;
    }
    backingOff_ = false;
    if (buffer_.empty()) {
        return;
    }
    const TelemetryReading& reading = buffer_.front();
    snprintf(inFlightEventId_, sizeof(inFlightEventId_), "%s", reading.eventId);
    if (ble_.isAvailable() && ble_.send(reading)) {
        bleInFlight_ = true;
        return;
    }
    if (edge_.isAvailable()) {
        complete(inFlightEventId_, edge_.send(reading));
    }
}

void BandConnectivityService::complete(const char* eventId, SendOutcome outcome) {
    if (outcome == SendOutcome::Retry) {
        backingOff_ = true;
        failedAtMs_ = millis();
        return;
    }
    if (!buffer_.empty() && strcmp(buffer_.front().eventId, eventId) == 0) {
        buffer_.pop();
    }
}
