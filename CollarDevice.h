#ifndef COLLAR_DEVICE_H
#define COLLAR_DEVICE_H

#include "ModestIoT.h"

#include "ActivityIndexCalculator.h"
#include "CollarClock.h"
#include "HeartRateEstimator.h"
#include "SendOutcome.h"
#include "TemperatureSensor.h"

// Mediator of the collar: the framework sensors report to it, and it queues one reading per interval.
class CollarDevice : public Device {
public:
    CollarDevice(WiFiConnectivityDriver& wifi, HttpGatewayClient& gateway);

    void on(Event event) override;

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override;
    void maintainNetworkServices() const override;

private:
    // Identifiers of the two schedules this device owns; the framework sensors use 20 to 23.
    static constexpr int kPulseSampleEvent = 100;
    static constexpr int kReadingEvent = 101;

    void buildAndQueueReading();
    SendOutcome post(const char* body) const;
    void replayBufferedBodies() const;

    WiFiConnectivityDriver& wifi_;
    HttpGatewayClient& gateway_;
    TemperatureSensor temperature_;
    ImuSensor motion_;
    GeolocationSensor gps_;
    GasSensor gas_;
    HeartRateEstimator heart_;
    ActivityIndexCalculator activity_;
    CollarClock clock_;
    bool motionFailed_ = false;
    uint32_t gpsUpdatedAtMs_ = 0;
    // Written by maintainNetworkServices(), which is const because the framework calls it on the network task.
    mutable bool wasConnected_ = false;
    mutable bool ntpStarted_ = false;
};

#endif  // COLLAR_DEVICE_H
