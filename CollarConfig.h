#ifndef COLLAR_CONFIG_H
#define COLLAR_CONFIG_H

#include <stddef.h>
#include <stdint.h>

// A fresh clone builds with the development values until CollarSecrets.h exists.
#if __has_include("CollarSecrets.h")
#include "CollarSecrets.h"
#else
#include "CollarSecrets.example.h"
#endif

// Values that differ between the simulation and the hardware prototype live here.
namespace CollarConfig {

constexpr char kDeviceId[] = "9d4a5f3b-3a6e-4f84-8daa-4e5f6a7b8c93";
constexpr char kDeviceSerial[] = "ICHU-DEV-0001";

constexpr char kTelemetryReadingsPath[] = "/api/v1/monitoring/telemetry-readings";
constexpr char kNtpServer[] = "pool.ntp.org";

// BLE is the planned route to a portable gateway; the demo and Wokwi use Wi-Fi.
constexpr bool kBleEnabled = false;
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
// ADC1 pins, so the readings keep working while Wi-Fi is on.
constexpr uint8_t kPulsePin = 5;
constexpr uint8_t kGasPin = 6;

// Mirror the edge and backend BodyTemperature range, so a collar off the animal
// reports an invalid temperature instead of losing the whole reading.
constexpr float kMinBodyTemperatureC = 30.0f;
constexpr float kMaxBodyTemperatureC = 45.0f;

// Mirror the edge and backend HeartRate range.
constexpr float kMinHeartRateBpm = 30.0f;
constexpr float kMaxHeartRateBpm = 200.0f;

constexpr uint32_t kReadingIntervalMs = 10000;
constexpr uint32_t kMotionSampleIntervalMs = 50;
// 100 Hz keeps the beat timing error near 1% at a resting cattle heart rate.
constexpr uint32_t kPulseSampleIntervalMs = 10;
// Beats closer than this are noise or the dicrotic wave; gaps longer than this are a lost signal.
constexpr uint32_t kPulseRefractoryMs = 200;
constexpr uint32_t kPulseMaxIntervalMs = 3000;
constexpr size_t kPulseMinIntervals = 2;
constexpr float kPulseMinAmplitudeMv = 300.0f;
constexpr float kPulseEnvelopeDecayMv = 0.5f;
constexpr float kPulseHysteresisMv = 50.0f;
// The MQ-2 heater output drifts for about a minute after power-up.
constexpr uint32_t kGasWarmupMs = 60000;
// AO reaches the ADC through a divider that maps the 5 V module output to 3.3 V.
constexpr float kGasFullScaleMv = 3300.0f;
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
