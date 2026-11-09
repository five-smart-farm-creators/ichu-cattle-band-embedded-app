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
