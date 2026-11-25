#ifndef TELEMETRY_READING_PACKAGE_H
#define TELEMETRY_READING_PACKAGE_H

#include "TelemetryPackage.h"
#include "TelemetryReading.h"

// Carries one reading through the framework queue; the network task builds the Edge body from it.
class TelemetryReadingPackage : public TelemetryPackage {
public:
    explicit TelemetryReadingPackage(const TelemetryReading& reading) : reading_(reading) {}

    const TelemetryReading& reading() const {
        return reading_;
    }

    // The Edge body is built by TelemetryPayload, so the flat writer only names the event.
    void write(TelemetryWriter& writer) const override {
        writer.field("event_id", reading_.eventId);
    }

private:
    TelemetryReading reading_;
};

#endif  // TELEMETRY_READING_PACKAGE_H
