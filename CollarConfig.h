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

constexpr char kFirmwareVersion[] = "0.1.1";
constexpr char kDeviceSerial[] = "ICHU-DEV-0001";

constexpr char kTelemetryReadingsPath[] = "/api/v1/monitoring/telemetry-readings";
constexpr char kNtpServer[] = "pool.ntp.org";
// POSIX time zone: readings are time stamped in UTC.
constexpr char kTimeZone[] = "UTC0";

constexpr uint8_t kTemperaturePin = 4;
// Source id of the motion sensor; its I2C bus uses the board default pins, SDA 8 and SCL 9.
constexpr uint8_t kI2cSdaPin = 8;
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

// Every sampling period is a whole number of engine ticks.
constexpr uint32_t kSchedulerTickMs = 10;
constexpr uint8_t kTelemetryQueueLength = 10;
constexpr uint32_t kReadingIntervalMs = 10000;
// The framework's DS18B20 read blocks the sampler for about 750 ms, so it samples once per interval.
constexpr uint32_t kTemperatureSampleIntervalMs = 10000;
constexpr uint32_t kMotionSampleIntervalMs = 50;
constexpr uint32_t kGpsSampleIntervalMs = 100;
constexpr uint32_t kGasSampleIntervalMs = 1000;
// Ten seconds of failed motion reads at kMotionSampleIntervalMs.
constexpr uint8_t kMotionFaultReads = 200;
// 100 Hz keeps the beat timing error near 1% at a resting cattle heart rate.
constexpr uint32_t kPulseSampleIntervalMs = kSchedulerTickMs;
// Beats closer than this are noise or the dicrotic wave; gaps longer than this are a lost signal.
constexpr uint32_t kPulseRefractoryMs = 200;
constexpr uint32_t kPulseMaxIntervalMs = 3000;
constexpr size_t kPulseMinIntervals = 2;
constexpr float kPulseMinAmplitudeMv = 300.0f;
constexpr float kPulseEnvelopeDecayMv = 0.5f;
constexpr float kPulseHysteresisMv = 50.0f;
// The MQ-2 heater output drifts for about a minute after power-up.
constexpr uint32_t kGasWarmupMs = 60000;
// A position older than this is not reported.
constexpr uint32_t kGpsMaxAgeMs = 5000;
// NEO-6M reports HDOP, not an accuracy radius; UERE gives an approximate meter value.
constexpr float kGpsUereMeters = 5.0f;
// Readings that cannot be sent wait in LittleFS; the oldest one is dropped when this is full.
constexpr int kBufferCapacity = 60;

// Placeholder: RMS deviation (m/s^2) from 1 g that maps to index 100, until calibrated on animals.
constexpr float kActivityFullScale = 3.0f;

// Linear approximation of the Li-ion discharge curve between empty and full.
constexpr uint32_t kBatteryEmptyMillivolts = 3000;
constexpr uint32_t kBatteryFullMillivolts = 4200;
// The 1:2 divider halves the cell voltage before it reaches the ADC pin.
constexpr float kBatteryDividerRatio = 2.0f;
// Full-scale voltage of the raw ADC counts that PowerManager reads.
constexpr uint32_t kAdcFullScaleMillivolts = 2500;

}  // namespace CollarConfig

#endif  // COLLAR_CONFIG_H
