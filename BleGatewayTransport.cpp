#include "BleGatewayTransport.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <stdio.h>

#include "CollarConfig.h"
#include "TelemetryPayload.h"

namespace {

// Every notification carries a 3-byte ATT header inside the negotiated MTU.
constexpr uint16_t kAttHeaderBytes = 3;

}  // namespace

void BleGatewayTransport::begin() {
    BLEDevice::init(CollarConfig::kDeviceSerial);
    BLEDevice::setMTU(CollarConfig::kBleRequestedMtu);
    server_ = BLEDevice::createServer();
    server_->setCallbacks(this);

    BLEService* service = server_->createService(CollarConfig::kBleServiceUuid);
    // NimBLE adds the client characteristic configuration descriptor for notifications itself.
    telemetry_ = service->createCharacteristic(
        CollarConfig::kBleTelemetryUuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    telemetry_->setCallbacks(this);
    ack_ = service->createCharacteristic(CollarConfig::kBleAckUuid, BLECharacteristic::PROPERTY_WRITE);
    ack_->setCallbacks(this);
    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(CollarConfig::kBleServiceUuid);
    advertising->start();
}

bool BleGatewayTransport::isAvailable() const {
    return connected_ && subscribed_;
}

bool BleGatewayTransport::send(const TelemetryReading& reading) {
    char body[TelemetryPayload::kMaxLength];
    const size_t length = TelemetryPayload::serialize(reading, body, sizeof(body));
    // A notification longer than the negotiated MTU would be truncated, so such a reading goes over HTTP.
    if (length == 0 || length + kAttHeaderBytes > static_cast<size_t>(server_->getPeerMTU(connHandle_))) {
        return false;
    }
    snprintf(pendingEventId_, sizeof(pendingEventId_), "%s", reading.eventId);
    ackReceived_ = false;
    telemetry_->setValue(reinterpret_cast<const uint8_t*>(body), length);
    telemetry_->notify();
    inFlight_ = true;
    sentAtMs_ = millis();
    Serial.printf("[ble] sent %s, waiting for ack\n", pendingEventId_);
    return true;
}

bool BleGatewayTransport::pollOutcome(SendOutcome& outcome) {
    if (!inFlight_) {
        return false;
    }
    if (ackReceived_) {
        inFlight_ = false;
        outcome = SendOutcome::Delivered;
        Serial.printf("[ble] acknowledged %s\n", pendingEventId_);
        return true;
    }
    if (millis() - sentAtMs_ >= CollarConfig::kAckTimeoutMs) {
        inFlight_ = false;
        outcome = SendOutcome::Retry;
        Serial.printf("[ble] no ack for %s, will retry\n", pendingEventId_);
        return true;
    }
    return false;
}

void BleGatewayTransport::onConnect(BLEServer*, ble_gap_conn_desc* desc) {
    connHandle_ = desc->conn_handle;
    connected_ = true;
    Serial.println("[ble] gateway connected");
}

void BleGatewayTransport::onDisconnect(BLEServer*, ble_gap_conn_desc*) {
    connected_ = false;
    subscribed_ = false;
    Serial.println("[ble] gateway disconnected");
    server_->startAdvertising();
}

void BleGatewayTransport::onWrite(BLECharacteristic* characteristic, ble_gap_conn_desc*) {
    if (characteristic == ack_ && ack_->getValue() == pendingEventId_) {
        ackReceived_ = true;
    }
}

void BleGatewayTransport::onSubscribe(BLECharacteristic* characteristic, ble_gap_conn_desc*, uint16_t subValue) {
    if (characteristic != telemetry_) {
        return;
    }
    // The telemetry characteristic only offers notifications, so any subscription enables them.
    subscribed_ = subValue != 0;
    Serial.println(subscribed_ ? "[ble] gateway subscribed" : "[ble] gateway unsubscribed");
}
