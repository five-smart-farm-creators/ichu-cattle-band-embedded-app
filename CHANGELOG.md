# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Sensor readings for body temperature (DS18B20), movement (MPU-6050), position (NEO-6M GPS) and battery level.
- Activity index computed from accelerometer samples over each 10 s reporting interval.
- Buffer of 60 pending readings that are retried when a route is unavailable.
- Telemetry delivery over HTTP to the Edge API on Wi-Fi, and over BLE to the Portable Edge Gateway when a gateway is subscribed.
- Wokwi simulation with a NEO-6M chip whose sliders move the simulated collar.
- Persistent queue in NVS: pending readings survive reboots and power loss.
- `invalid_reasons` in the telemetry body for a body temperature, activity index or position that its sensor cannot provide.
- `build` job in the Git Flow checks workflow, which compiles the firmware on every pull request.
- Heart rate from an SP-203 pulse sensor (median beat interval per reporting interval) and a 0–100 methane index from an MQ-2 gas sensor, sent as `heart_rate` and `methane_index` with their own invalid reasons.
- Wokwi `pulse-sensor` chip with a heart rate slider, and the MQ-2 gas sensor in the simulated circuit.

### Fixed

- A body temperature outside 30–45 °C is sent in `invalid_reasons` as `Temperature out of physiological range` instead of making the edge reject the whole reading with 400.

### Changed

- Wi-Fi credentials, the Edge API address and the device API key move from `CollarConfig.h` to `CollarSecrets.h`, which is git-ignored; a fresh clone builds with the values of `CollarSecrets.example.h`.
- BLE is disabled by default (`kBleEnabled = false`); the demo and Wokwi use Wi-Fi.
