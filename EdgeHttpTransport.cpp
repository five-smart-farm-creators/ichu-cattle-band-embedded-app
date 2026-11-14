#include "EdgeHttpTransport.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "CollarConfig.h"
#include "TelemetryPayload.h"

void EdgeHttpTransport::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(CollarSecrets::kWifiSsid, CollarSecrets::kWifiPassword);
    Serial.printf("[wifi] connecting to %s\n", CollarSecrets::kWifiSsid);
}

void EdgeHttpTransport::poll() {
    const bool connected = isAvailable();
    if (connected == wasConnected_) {
        return;
    }
    wasConnected_ = connected;
    if (connected) {
        Serial.printf("[wifi] connected, IP %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[wifi] disconnected");
    }
}

bool EdgeHttpTransport::isAvailable() const {
    return WiFi.status() == WL_CONNECTED;
}

SendOutcome EdgeHttpTransport::send(const TelemetryReading& reading) {
    char body[TelemetryPayload::kMaxLength];
    const size_t length = TelemetryPayload::serialize(reading, body, sizeof(body));
    if (length == 0) {
        Serial.printf("[edge] %s exceeds the payload buffer, dropped\n", reading.eventId);
        return SendOutcome::Rejected;
    }

    const String url = String(CollarSecrets::kEdgeBaseUrl) + CollarConfig::kTelemetryReadingsPath;
    HTTPClient http;
    http.begin(url);
    http.setConnectTimeout(static_cast<int32_t>(CollarConfig::kHttpTimeoutMs));
    http.setTimeout(static_cast<uint16_t>(CollarConfig::kHttpTimeoutMs));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", CollarSecrets::kDeviceApiKey);
    const int status = http.POST(String(body));
    http.end();

    switch (status) {
        case 200:
        case 201:
            Serial.printf("[edge] delivered %s (%d)\n", reading.eventId, status);
            return SendOutcome::Delivered;
        case 400:
        case 409:
            Serial.printf("[edge] rejected %s (%d), resending the same body cannot succeed\n", reading.eventId, status);
            return SendOutcome::Rejected;
        default:
            // Timeouts (negative codes), 401 and 5xx may clear up, so the reading stays queued.
            Serial.printf("[edge] retry %s (%d)\n", reading.eventId, status);
            return SendOutcome::Retry;
    }
}
