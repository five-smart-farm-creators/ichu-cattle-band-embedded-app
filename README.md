# ICHU Cattle Band Embedded Application

Firmware of the ICHU cattle band, the collar that monitors each animal. It runs on an ESP32-S3 DevKitC-1, reads body temperature, movement, position and battery level, computes an activity index, and sends every reading to the ICHU back end.

---

## Overview

The band is the device edge of the ICHU solution (report, section 5.6). For each reading it chooses one of two routes:

- **BLE** to the Portable Edge Gateway, when a gateway is connected and has subscribed to notifications. It is off by default (see [BLE Contract](#ble-contract)).
- **HTTP over Wi-Fi** to the Edge API, otherwise.

The HTTP body is the contract of `POST /api/v1/monitoring/telemetry-readings` in `ichu-edge-service`; the BLE route carries the same JSON. A sensor that does not answer is reported in `invalid_reasons` (see [Invalid Readings](#invalid-readings)). Readings that cannot be sent wait in a queue of 60 readings kept in NVS and are retried.

Wokwi cannot simulate BLE between two devices, so the simulation demonstrates the Wi-Fi route. The gateway side of BLE is a later phase of `ichu-edge-service`.

---

## Hardware

| Part | Interface | ESP32-S3 pin | Notes |
| --- | --- | --- | --- |
| DS18B20 temperature probe | 1-Wire (DQ) | GPIO4 | 4.7 kΩ pull-up between DQ and 3V3 |
| MPU-6050 accelerometer | I2C (SDA, SCL) | GPIO8, GPIO9 | Address 0x68, range ±4 g |
| NEO-6M GPS module | UART1, 9600 baud | GPIO18 receives the module TX, GPIO17 drives the module RX | NMEA 0183 sentences |
| Li-ion battery | ADC through a 1:2 divider | GPIO1 | Two equal resistors halve the cell voltage |

All modules share the 3V3 and GND rails of the board.

---

## Project Structure

```text
ichu-cattle-band-embedded-app/
├── sketch.ino                       composition root: setup() and loop()
├── CollarConfig.h                   identity, pins, intervals and thresholds
├── CollarSecrets.example.h          template for the git-ignored CollarSecrets.h
├── TelemetryReading.h               reading value type with optional measurements
├── SendOutcome.h                    result of one send attempt
├── ActivityIndexCalculator.h/.cpp   activity index from accelerometer samples
├── ReadingBuffer.h/.cpp             FIFO of pending readings, kept in NVS
├── TemperatureSensor.h/.cpp         DS18B20 over 1-Wire
├── MotionSensor.h/.cpp              MPU-6050 accelerometer
├── GpsReceiver.h/.cpp               NEO-6M over UART with TinyGPS++
├── BatteryMonitor.h/.cpp            battery level through the divider
├── CollarClock.h/.cpp               UTC clock from NTP or GPS, event ids
├── TelemetryPayload.h/.cpp          JSON body of the Edge API contract
├── EdgeHttpTransport.h/.cpp         Wi-Fi and HTTP POST to the Edge API
├── BleGatewayTransport.h/.cpp       BLE GATT peripheral towards the gateway
├── BandConnectivityService.h/.cpp   chooses the route and drains the buffer
├── gps-neo6m.chip.c                 Wokwi NEO-6M chip
├── gps-neo6m.chip.json              chip pins and position sliders
├── gps-neo6m.chip.wasm              compiled chip loaded by Wokwi
├── diagram.json                     Wokwi circuit
├── libraries.txt                    Wokwi library list
├── wokwi.toml                       Wokwi for VS Code configuration
├── wokwi-project.txt                Wokwi project link (pending)
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE.md
└── README.md
```

The sources stay at the root so that the project also opens in the Wokwi web editor.

---

## Configuration

Copy `CollarSecrets.example.h` to `CollarSecrets.h` and set the values there. The copy is git-ignored, and a fresh clone builds with the example values, which target Wokwi.

- `kDeviceApiKey` must equal the `DEVICE_API_KEY` or `DEVELOPMENT_DEVICE_API_KEY` registered in the Edge API.
- `kWifiSsid` and `kWifiPassword` select the network. The ESP32-S3 joins only 2.4 GHz networks.
- `kEdgeBaseUrl` points to the Edge API. The Wokwi value reaches the host through `host.wokwi.internal`. On hardware, set it to the PC LAN address, for example `http://192.168.1.20:5000`, and start the Edge API with `HOST=0.0.0.0` so that it listens on the LAN.

Edit `CollarConfig.h` for the rest:

- `kDeviceId` and `kDeviceSerial` identify the development collar.
- `kBleEnabled` turns the BLE route on or off. It is off by default.
- Pins, intervals, the activity full scale and the battery thresholds are in the same file.

---

## Build

Every change must compile for `esp32:esp32:esp32s3` without warnings from the project files. arduino-cli expects the main file to match its folder name, so the sources are copied into a folder named `sketch` outside the repository:

```sh
arduino-cli core install esp32:esp32@3.3.4
arduino-cli lib install ArduinoJson OneWire DallasTemperature "Adafruit MPU6050" "Adafruit Unified Sensor" TinyGPSPlus
mkdir -p ../build-src/sketch
cp *.ino *.h *.cpp ../build-src/sketch/
arduino-cli compile --fqbn esp32:esp32:esp32s3 --warnings all --output-dir build ../build-src/sketch
```

To flash a board, run `arduino-cli upload --port <port> --fqbn esp32:esp32:esp32s3 ../build-src/sketch`. The `build/` folder is ignored by Git.

The default partition scheme gives the application 1310720 bytes; the current firmware uses 1303211 bytes (99%), so the next feature may not fit. The same build with `--board-options PartitionScheme=huge_app` (3 MB application partition, "Huge APP (3MB No OTA/1MB SPIFFS)" in the Arduino IDE) uses 1303243 bytes, 41% of the partition.

---

## Simulation in Wokwi

**Wokwi for VS Code.** `wokwi.toml` points to `build/sketch.ino.bin` and `build/sketch.ino.elf`, which the build above produces, and declares the custom chip `gps-neo6m`. The chip binary `gps-neo6m.chip.wasm` is committed, so a fresh clone runs the GPS without extra tools. After changing `gps-neo6m.chip.c`, rebuild it with `wokwi-cli chip compile gps-neo6m.chip.c -o gps-neo6m.chip.wasm` and commit both files; the `wokwi-api.h` that the command downloads is git-ignored.

**Wokwi web editor.** The project link will be recorded in `wokwi-project.txt` once the team publishes it.

`diagram.json` wires the parts of the table above to the board. The `gps-neo6m` chip sends one `$GPRMC` and one `$GPGGA` sentence per second with a valid fix (HDOP 0.9, 8 satellites). Its `northMeters` and `eastMeters` sliders move the position from the development geofence center by up to ±2000 m, which takes the animal out of the geofence during a demo.

---

## BLE Contract

| Item | Value |
| --- | --- |
| Device name | `ICHU-DEV-0001` (the device serial) |
| Service | `d81475bb-901c-414f-ad60-f30c02ea1ee7` |
| Telemetry characteristic (READ, NOTIFY) | `59e9bd24-b52a-4272-825a-2e71ededcc15`: value is the same JSON body as the HTTP request |
| Acknowledgement characteristic (WRITE) | `857df400-45dd-4595-acd0-71f5e08d6253`: the gateway writes back the `event_id` |
| Requested MTU | 517 bytes |

The band keeps one reading in flight. It notifies the telemetry characteristic and waits up to 5 s for the gateway to write the reading's `event_id` to the acknowledgement characteristic. The gateway writes it once the edge has stored the reading or rejected it permanently; both cases remove the reading from the band buffer. Without an acknowledgement in time, the band sends the same reading again with the same `event_id`, so the edge answers `200 OK` for an identical retry instead of storing a duplicate.

BLE is off by default (`kBleEnabled = false`): it is the planned route to a portable gateway, and the demo and Wokwi use Wi-Fi. When it is on, the BLE route is used only while a gateway is connected and has subscribed to notifications, and only for a body that fits the negotiated MTU minus the 3-byte ATT header (typical bodies are about 300 bytes). A reading that does not fit goes over HTTP.

---

## Invalid Readings

A sensor that does not answer does not block its reading: the reading is still sent, with the failure listed in `invalid_reasons` instead of a value. A field with a reason carries no value.

| Field | Reason sent to the Edge API |
| --- | --- |
| `body_temperature` | Temperature sensor not responding |
| `activity_index` | Motion sensor not responding |
| `position` | GPS receiver not responding |

- The temperature sensor fails when the DS18B20 read fails.
- A DS18B20 value outside 30–45 °C, the range the edge accepts, is sent as `Temperature out of physiological range`, so a collar off the animal still delivers its other measurements.
- The motion sensor fails when the MPU-6050 gives no sample during the whole reporting interval.
- The GPS fails only when the NEO-6M sends no NMEA data at all. A GPS that answers without a fix leaves `position` absent, which is not an invalid reason.

---

## Serial Output

The firmware prints one summary line per reading. Angle brackets mark values that come from the sensors at run time:

```text
[wifi] connecting to Wokwi-GUEST
[wifi] connected, IP <address>
[clock] NTP sync started
[telemetry] event=<serial>-<boot>-<sequence> temp=<celsius>C activity=<0-100> battery=<percent>% position=<latitude>,<longitude> queued=<count>
[edge] delivered <event id> (201)
```

A failed sensor prints `temp=failed`, `activity=failed` or `position=failed` in place of its value, and a temperature outside 30–45 °C prints `temp=out-of-range`. A restart with queued readings prints `[buffer] restored <count> readings`. Other lines use the prefixes `[ble]`, `[buffer]`, `[gps]`, `[sensors]` and `[edge]`.

---

## Activity Index

Each MPU-6050 sample is taken every 50 ms. Over one 10 s reporting interval the firmware computes:

```text
activity index = clamp(100 × mean(| |a| − 9.80665 |) / 3.0, 0, 100)
```

where `a = (ax, ay, az)` is the acceleration in m/s². Movement appears as deviation from gravity, so a resting animal scores near zero. The full scale of 3.0 m/s² is a placeholder until the team calibrates it on animals, so the index is not comparable with other scales yet.

---

## Known Limitations

- The queue of 60 readings in NVS survives reboots and power loss. A cut during a write can lose the reading being queued, and can resend a reading that was already delivered; the edge drops the resend by `event_id`. The boot counter in flash keeps event ids unique across reboots.
- The BLE link is neither authenticated nor encrypted yet.
- Wokwi has no BLE between devices, so the simulation exercises only the Wi-Fi route. `kBleEnabled` is `false` by default.
- The DS18B20 measures surface temperature, not core body temperature.
- An HTTP request blocks the loop for at most `kHttpTimeoutMs`, and each DS18B20 conversion blocks for about 190 ms.
- The battery percentage is a linear approximation of the Li-ion discharge curve.
- The position accuracy is an approximation derived from HDOP.
- The physical prototype has not been tested yet.

---

## License

See [LICENSE.md](LICENSE.md).
