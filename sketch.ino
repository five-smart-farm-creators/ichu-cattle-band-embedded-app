// Composition root: builds the framework's Wi-Fi and HTTP transports and the collar device.
#include <Arduino.h>

#include "CollarConfig.h"
#include "CollarDevice.h"
#include "ModestIoT.h"

static WiFiConnectivityDriver* wifiDriver = nullptr;
static HttpGatewayClient* edgeGateway = nullptr;
static CollarDevice* collar = nullptr;
// The framework keeps the pointers it is given, so the strings must outlive setup().
static String edgeTelemetryUrl;
static String apiKeyAuthorization;

void setup() {
    Serial.begin(115200);
    Device::setFirmwareVersion(CollarConfig::kFirmwareVersion);

    PowerManager::begin(CollarConfig::kBatteryPin, CollarConfig::kBatteryDividerRatio,
                        CollarConfig::kAdcFullScaleMillivolts);
    PowerManager::setBatteryVoltageRange(CollarConfig::kBatteryEmptyMillivolts,
                                         CollarConfig::kBatteryFullMillivolts);

    if (TelemetryBuffer::begin(CollarConfig::kBufferCapacity)) {
        TelemetryBuffer::onEviction([]() {
            Serial.println("[buffer] full, dropped the oldest reading");
        });
        const int restored = TelemetryBuffer::getBufferedPacketCount();
        if (restored > 0) {
            Serial.printf("[buffer] restored %d readings\n", restored);
        }
    } else {
        MODEST_LOG_W("Collar", "telemetry buffer unavailable, readings are lost while offline");
    }

    edgeTelemetryUrl = String(CollarSecrets::kEdgeBaseUrl) + CollarConfig::kTelemetryReadingsPath;
    apiKeyAuthorization = String("ApiKey ") + CollarSecrets::kDeviceApiKey;
    wifiDriver = new WiFiConnectivityDriver(CollarSecrets::kWifiSsid, CollarSecrets::kWifiPassword);
    edgeGateway = new HttpGatewayClient(*wifiDriver, edgeTelemetryUrl.c_str(), HttpGatewayClient::HttpMethod::Post);
    edgeGateway->setAuthorizationHeader(apiKeyAuthorization.c_str());
    collar = new CollarDevice(*wifiDriver, *edgeGateway);

    Serial.printf("[wifi] connecting to %s\n", CollarSecrets::kWifiSsid);
    wifiDriver->connect();
}

void loop() {
    // Sampling, networking and telemetry run on the framework's tasks; loop() only yields.
    vTaskDelay(pdMS_TO_TICKS(60000));
}
