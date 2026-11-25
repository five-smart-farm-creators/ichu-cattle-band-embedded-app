# ICHU Cattle Band Embedded Application

Firmware of the ICHU cattle band, the collar that monitors each animal. It runs on an ESP32-S3 DevKitC-1, reads body temperature, movement, position, heart rate, a methane index and battery level, computes an activity index, and sends every reading to the ICHU Edge API. It is built on the [Modest IoT Nano Framework](https://github.com/modest-iot/modest-iot-nano-framework) v1.3.0.

---

## Overview

The band is the device edge of the ICHU solution (report, section 5.6). Every 10 s it sends one reading over Wi-Fi to the Edge API. The JSON body is the contract of `POST /api/v1/monitoring/telemetry-readings` in `ichu-edge-service`; the request carries the device key as `Authorization: ApiKey <key>`.

A sensor that does not answer is reported in `invalid_reasons` (see [Invalid Readings](#invalid-readings)). Readings that cannot be sent wait in a LittleFS queue of 60 readings and are replayed after the next successful send. BLE is not part of the firmware.

### Framework structure

- `CollarDevice` is the mediator: a `Device` subclass that receives the events of the framework sensors and queues one `TelemetryReadingPackage` per interval.
- The sensors run on the framework's sampler task, registered with `appendSensorToScheduler()` before `initializeAsynchronousEngine()`.
- The framework's network task calls `processQueuedTelemetryData()`, which builds the Edge body with `TelemetryPayload` and posts it with `HttpGatewayClient::sendRequest()`.
- `maintainNetworkServices()` tracks the Wi-Fi link and starts NTP through `TimeSynchronizationDriver`.
- `loop()` only yields to FreeRTOS.

Framework classes in use: `Device`, `ImuSensor`, `GeolocationSensor`, `GasSensor`, `PowerManager`, `TelemetryBuffer`, `TelemetryPackage`, `WiFiConnectivityDriver`, `HttpGatewayClient` and `TimeSynchronizationDriver`. The DS18B20 keeps the DallasTemperature driver, because the framework `OneWireTemperatureSensor` misreads it in Wokwi.

---

## Hardware

| Part | Interface | ESP32-S3 pin | Notes |
| --- | --- | --- | --- |
| DS18B20 temperature probe | 1-Wire (DQ) | GPIO4 | 4.7 kΩ pull-up between DQ and 3V3 |
| MPU-6050 accelerometer | I2C (SDA, SCL) | GPIO8, GPIO9 | Board default I2C pins; address 0x68, range ±4 g |
| NEO-6M GPS module | UART1, 9600 baud | GPIO18 receives the module TX, GPIO17 drives the module RX | NMEA 0183 sentences |
| Li-ion battery | ADC through a 1:2 divider | GPIO1 | Two equal resistors halve the cell voltage |
| SP-203 pulse sensor | Analog (ADC1) | GPIO5 | Photoelectric PPG output around mid-rail |
| MQ-2 gas sensor | Analog AO (ADC1) | GPIO6 | 5 V module; AO needs a divider to 3.3 V; DO unused |

All modules except the MQ-2 share the 3V3 and GND rails of the board. The MQ-2 heater runs on 5 V and draws about 150 mA, which shortens the battery life of a collar considerably; it is a prototype choice, and the Wokwi part runs on 3V3 without the divider.

---

## Project Structure

```text
ichu-cattle-band-embedded-app/
├── sketch.ino                       composition root: setup() builds the device, loop() only yields
├── CollarConfig.h                   identity, pins, intervals and thresholds
├── CollarSecrets.example.h          template for the git-ignored CollarSecrets.h
├── CollarDevice.h/.cpp              Device mediator: framework sensors, readings, Edge delivery
├── TelemetryReadingPackage.h        TelemetryPackage that carries one reading through the queue
├── TelemetryReading.h               reading value type with optional measurements
├── TelemetryPayload.h/.cpp          JSON body of the Edge API contract
├── SendOutcome.h                    result of one send attempt
├── ActivityIndexCalculator.h/.cpp   activity index from the motion samples
├── HeartRateEstimator.h/.cpp        beat detection and heart rate from the SP-203
├── CollarClock.h/.cpp               event ids, UTC formatting and GPS time while offline
├── gps-neo6m.chip.c                 Wokwi NEO-6M chip
├── gps-neo6m.chip.json              chip pins and position sliders
├── gps-neo6m.chip.wasm              compiled chip loaded by Wokwi
├── pulse-sensor.chip.c              Wokwi SP-203 pulse sensor chip
├── pulse-sensor.chip.json           chip pins and heart rate slider
├── pulse-sensor.chip.wasm           compiled pulse sensor chip
├── diagram.json                     Wokwi circuit
├── libraries.txt                    Wokwi library list
├── wokwi.toml                       Wokwi for VS Code configuration
├── wokwi-project.txt                Wokwi project link
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE.md
└── README.md
```

The sources stay at the root so that the project also opens in the Wokwi web editor.

---

## Configuration

Copy `CollarSecrets.example.h` to `CollarSecrets.h` and set the values there. The copy is git-ignored, and a fresh clone builds with the example values, which target Wokwi.

- `kDeviceId` is the id that the ICHU service issued when the collar was registered; the example holds the development collar.
- `kDeviceApiKey` must equal the `DEVICE_API_KEY` or `DEVELOPMENT_DEVICE_API_KEY` registered in the Edge API.
- `kWifiSsid` and `kWifiPassword` select the network. The ESP32-S3 joins only 2.4 GHz networks.
- `kEdgeBaseUrl` points to the Edge API. The Wokwi value reaches the host through `host.wokwi.internal`. On hardware, set it to the PC LAN address, for example `http://192.168.1.20:5000`, and start the Edge API with `HOST=0.0.0.0` so that it listens on the LAN.

Edit `CollarConfig.h` for the rest:

- `kDeviceSerial` names the collar in event ids.
- `kFirmwareVersion` is set with `Device::setFirmwareVersion()`; it is not part of the Edge body.
- Pins, sampling intervals, the activity full scale and the battery range are in the same file.

---

## Build

Every change must compile for `esp32:esp32:esp32s3` without warnings from the project files. arduino-cli expects the main file to match its folder name, so the sources are copied into a folder named `sketch` outside the repository:

```sh
arduino-cli core install esp32:esp32@3.3.4
arduino-cli lib install ArduinoJson Modest-IoT-Nano-Framework MQTT
mkdir -p ../build-src/sketch
cp *.ino *.h *.cpp ../build-src/sketch/
arduino-cli compile --fqbn esp32:esp32:esp32s3 --warnings all --output-dir build ../build-src/sketch
```

To flash a board, run `arduino-cli upload --port <port> --fqbn esp32:esp32:esp32s3 ../build-src/sketch`. The `build/` folder is ignored by Git.

The default partition scheme gives the application 1310720 bytes. The firmware before the framework migration used 1303211 bytes (99%). The size of the migrated build is not measured yet; if the compile reports that the sketch is too big, build with `--board-options PartitionScheme=huge_app` (3 MB application partition).

---

## Simulation in Wokwi

**Wokwi for VS Code.** `wokwi.toml` points to `build/sketch.ino.bin` and `build/sketch.ino.elf`, which the build above produces, and declares the custom chips `gps-neo6m` and `pulse-sensor`. Both chip binaries are committed, so a fresh clone runs them without extra tools. After changing a chip's `.c` file, rebuild it with `wokwi-cli chip compile <chip>.chip.c -o <chip>.chip.wasm` and commit both files; the `wokwi-api.h` that the command downloads is git-ignored.

**Automated simulation.** The `build` job of the Git Flow checks runs `tests/collar-sensors.test.yaml` in Wokwi CI after compiling. The scenario waits for the gas warm-up reason, sets the pulse slider to 0 and to 220, and expects `heart=no-contact` and `heart=out-of-range`. It needs a `WOKWI_CLI_TOKEN` repository secret, created in the Wokwi CI dashboard; without it the step is skipped. The `end-to-end` job starts the Edge API on the runner and reaches it from the simulated collar through a public `bore.pub` tunnel and a throwaway device key. It raises the DS18B20 control to 41 °C, a fever, and fails unless the Edge API then lists a Critical Temperature alert for the collar. The Edge API must accept `Authorization: ApiKey <key>` on its `develop` branch for this job to deliver readings.

**Wokwi web editor.** Open the published project at <https://wokwi.com/projects/480787162969005057>. The web editor shows the circuit and runs the firmware, but it reaches a local Edge API only with a paid private gateway, so the connected demo runs in Wokwi for VS Code.

`diagram.json` wires the parts of the table above to the board. The `gps-neo6m` chip sends one `$GPRMC` and one `$GPGGA` sentence per second with a valid fix (HDOP 0.9, 8 satellites). Its `northMeters` and `eastMeters` sliders move the position from the development geofence center by up to ±2000 m, which takes the animal out of the geofence during a demo.

Wokwi has no pulse sensor part, so the `pulse-sensor` chip outputs a PPG wave at 100 Hz: a systolic peak and a smaller dicrotic wave each beat. Its `bpm` slider sets the heart rate from 0 to 240; 0 means no skin contact and leaves the output flat. `pulse-sensor.chip.wasm` is compiled from `pulse-sensor.chip.c` with `wokwi-cli chip compile`. The MQ-2 is the built-in `wokwi-gas-sensor`; its `ppm` control raises the AO voltage.

---

## Invalid Readings

A sensor that does not answer does not block its reading: the reading is still sent, with the failure listed in `invalid_reasons` instead of a value. A field with a reason carries no value.

| Field | Reason sent to the Edge API |
| --- | --- |
| `body_temperature` | Temperature out of physiological range |
| `activity_index` | Motion sensor not responding |
| `heart_rate` | Pulse sensor without contact, or Heart rate out of physiological range |
| `methane_index` | Gas sensor warming up |

- A DS18B20 value outside 30–45 °C, the range the edge accepts, is sent as `Temperature out of physiological range`, so a collar off the animal still delivers its other measurements. A disconnected probe is expected to read −0.0625 °C and takes the same path (not verified on hardware).
- The motion sensor fails when 200 consecutive reads, about 10 s at 50 ms, do not succeed. A quiet sensor is not a failure.
- The pulse sensor has no contact when fewer than two beat intervals were detected in the reporting interval. A heart rate outside 30–200 bpm, the range the edge accepts, is sent as `Heart rate out of physiological range`.
- The MQ-2 needs about 60 s for its heater to settle, so readings in the first minute after boot send `Gas sensor warming up`.
- A position is sent only when the GPS has a fix and its last update is at most 5 s old. Otherwise `position` is absent, and no reason is sent.

---

## Serial Output

The firmware prints one summary line per reading. Angle brackets mark values that come from the sensors at run time:

```text
[wifi] connecting to <network>
[wifi] connected, IP <address>
[clock] NTP sync started
[telemetry] event=<serial>-<boot>-<sequence> temp=<celsius>C activity=<0-100> battery=<percent>% heart=<bpm>bpm methane=<0-100> position=<latitude>,<longitude>
[edge] delivered <event id> (201)
```

A temperature outside 30–45 °C prints `temp=out-of-range`, and a motion sensor that has stopped answering prints `activity=failed`. The pulse sensor prints `heart=no-contact` or `heart=out-of-range`, and the gas sensor prints `methane=warming-up` during its first minute. A restart with queued readings prints `[buffer] restored <count> readings`, and a delivered backlog prints `[buffer] sent <count> buffered readings`. `[edge] retry` and `[edge] rejected` report the HTTP status of a failed send. Other lines use the prefixes `[buffer]`, `[clock]`, `[edge]`, `[telemetry]` and `[wifi]`.

---

## Activity Index

The MPU-6050 is read every 50 ms through the framework's `ImuSensor`. It reports a reading only when an axis changes by more than 0.01 g, so the samples of a reporting interval are the changed readings; a quiet animal has none and scores zero. For the samples `a_i` in one 10 s reporting interval the firmware computes:

```text
deviation_i = (|a_i| / 1 g − 1) × 9.80665 m/s²
activity index = clamp(100 × sqrt(mean(deviation_i²)) / 3.0, 0, 100)
```

where `|a_i|` is the magnitude of the acceleration vector in g. Movement appears as deviation from gravity, so a resting animal scores near zero. The full scale of 3.0 m/s² is a placeholder until the team calibrates it on animals, so the index is not comparable with other scales yet.

---

## Heart Rate and Methane Index

The SP-203 output is sampled every 10 ms. A peak and a trough envelope follow the signal, decaying 0.5 mV per sample, and a beat is the rising crossing of their midpoint, with 50 mV of hysteresis. A signal with less than 300 mV between envelopes counts as no contact. Beats closer than 200 ms are ignored, and gaps longer than 3 s are not used as intervals. Over each 10 s reporting interval:

```text
heart rate = 60000 / median(beat intervals in ms)
```

The median keeps one missed or doubled beat from skewing the rate. The beat detector is the project's own `HeartRateEstimator`, not the framework's `PulseSensor`, which drops rates above 200 bpm before they reach the device.

The MQ-2 AO pin is read as raw 12-bit ADC counts through the framework's `GasSensor`, which maps linearly to a relative index:

```text
methane index = clamp(100 × ADC counts / 4095, 0, 100)
```

The MQ-2 is neither calibrated nor selective (it also reacts to LPG, propane, hydrogen and alcohol), so the index shows change over time and is **not** a concentration in ppm.

---

## Known Limitations

- The queue in LittleFS survives reboots and power loss. The framework writes one file per reading, so a cut during a write can lose the reading being stored. The queue holds 60 readings; the oldest one is dropped when it is full.
- The DS18B20 driver is the framework's native 1-Wire implementation. Its read slot releases the bus without the 1 µs low pulse that the DS18B20 datasheet requires; it has not been verified on the prototype.
- The framework's sensors do not report read failures for the temperature probe or the GPS receiver, so `Temperature sensor not responding` and `GPS receiver not responding` are no longer sent.
- The framework's NMEA parser skips empty fields, so a sentence with empty fields can shift its values. A position is used only with a fix.
- The framework stores coordinates as `float`; in the longitude range of Peru one step is about 0.8 m.
- The battery reading assumes a raw ADC full scale of 2500 mV at the core's default attenuation; the value is not measured yet.
- A DS18B20 conversion blocks the framework's sampler for about 750 ms every 10 s, so pulse samples are missed during that time. Gaps longer than 3 s are not used as beat intervals, and the median absorbs a delayed beat.
- The DS18B20 measures surface temperature, not core body temperature.
- The position accuracy is an approximation derived from HDOP.
- The SP-203 is a fingertip sensor; reading it through cattle skin and hair is unproven.
- The physical prototype has not been tested yet.

---

## License

See [LICENSE.md](LICENSE.md).
