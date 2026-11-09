#ifndef COLLAR_CONFIG_H
#define COLLAR_CONFIG_H

#include <stddef.h>
#include <stdint.h>

// Values that differ between the simulation and the hardware prototype live here.
namespace CollarConfig {

// The API key must equal DEVELOPMENT_DEVICE_API_KEY of the Edge API.
constexpr char kDeviceId[] = "9d4a5f3b-3a6e-4f84-8daa-4e5f6a7b8c93";
constexpr char kDeviceSerial[] = "ICHU-DEV-0001";
constexpr char kDeviceApiKey[] = "development-device-key";

// Wokwi reaches the host through this gateway name; hardware needs the PC LAN address.
constexpr char kWifiSsid[] = "Wokwi-GUEST";
constexpr char kWifiPassword[] = "";
constexpr char kEdgeBaseUrl[] = "http://host.wokwi.internal:5000";
constexpr char kTelemetryReadingsPath[] = "/api/v1/monitoring/telemetry-readings";
constexpr char kNtpServer[] = "pool.ntp.org";

// Wokwi may not start the BLE controller; set it to false to run the Wi-Fi route only.
constexpr bool kBleEnabled = true;
constexpr char kBleServiceUuid[] = "d81475bb-901c-414f-ad60-f30c02ea1ee7";
constexpr char kBleTelemetryUuid[] = "59e9bd24-b52a-4272-825a-2e71ededcc15";
constexpr char kBleAckUuid[] = "857df400-45dd-4595-acd0-71f5e08d6253";
constexpr uint16_t kBleRequestedMtu = 517;

constexpr uint8_t kTemperaturePin = 4;
constexpr uint8_t kI2cSdaPin = 8;
constexpr uint8_t kI2cSclPin = 9;
constexpr uint8_t kGpsRxPin = 18;
constexpr uint8_t kGpsTxPin = 17;
constexpr uint32_t kGpsBaud = 9600;
constexpr uint8_t kBatteryPin = 1;

constexpr uint32_t kReadingIntervalMs = 10000;
constexpr uint32_t kMotionSampleIntervalMs = 50;
constexpr uint32_t kGpsMaxAgeMs = 5000;
// Silence longer than this usually means the GPS module is not wired.
constexpr uint32_t kGpsSilenceTimeoutMs = 5000;
constexpr uint32_t kAckTimeoutMs = 5000;
constexpr uint32_t kHttpTimeoutMs = 5000;
// A pause after each failed attempt keeps an unreachable edge from being hammered.
constexpr uint32_t kRetryBackoffMs = 5000;
constexpr size_t kBufferCapacity = 60;

// Placeholder: mean deviation (m/s^2) that maps to index 100, until calibrated on animals.
constexpr float kActivityFullScale = 3.0f;
// NEO-6M reports HDOP, not an accuracy radius; UERE gives an approximate meter value.
constexpr float kGpsUereMeters = 5.0f;

// Linear approximation of the Li-ion discharge curve between empty and full.
constexpr float kBatteryEmptyVolts = 3.0f;
constexpr float kBatteryFullVolts = 4.2f;
// The 1:2 divider halves the cell voltage before it reaches the ADC pin.
constexpr float kBatteryDividerRatio = 2.0f;

}  // namespace CollarConfig

#endif  // COLLAR_CONFIG_H
