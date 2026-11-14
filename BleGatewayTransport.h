#ifndef BLE_GATEWAY_TRANSPORT_H
#define BLE_GATEWAY_TRANSPORT_H

#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <stdint.h>

#include "SendOutcome.h"
#include "TelemetryReading.h"

// GATT peripheral towards the Portable Edge Gateway; one reading is in flight at a time.
class BleGatewayTransport : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    void begin();
    bool isAvailable() const;
    // Returns false when the reading cannot go over BLE now; the caller then falls back to HTTP.
    bool send(const TelemetryReading& reading);
    // Reports the reading in flight once its ack arrives or kAckTimeoutMs expires.
    bool pollOutcome(SendOutcome& outcome);

    void onConnect(BLEServer*, ble_gap_conn_desc* desc) override;
    void onDisconnect(BLEServer*, ble_gap_conn_desc*) override;
    void onWrite(BLECharacteristic* characteristic, ble_gap_conn_desc*) override;
    void onSubscribe(BLECharacteristic* characteristic, ble_gap_conn_desc*, uint16_t subValue) override;

private:
    BLEServer* server_ = nullptr;
    BLECharacteristic* telemetry_ = nullptr;
    BLECharacteristic* ack_ = nullptr;
    // Written by the BLE stack's callbacks and read by loop().
    volatile bool connected_ = false;
    volatile bool subscribed_ = false;
    volatile bool ackReceived_ = false;
    uint16_t connHandle_ = 0;
    bool inFlight_ = false;
    // A copy, so the ack callback never reads the buffer while loop() may change it.
    char pendingEventId_[TelemetryReading::kEventIdCapacity] = "";
    uint32_t sentAtMs_ = 0;
};

#endif  // BLE_GATEWAY_TRANSPORT_H
