# Implementation Plan: RS232-WiFi Beamer Adapter

**Branch**: `001-rs232-wifi-adapter` | **Date**: 2026-05-22 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/001-rs232-wifi-adapter/spec.md`

## Summary

Build PlatformIO/Arduino-ESP32 firmware for the ESP32-S3 DevKitC-1 N16R8 that bridges
WiFi (REST API + MQTT + Web UI) to RS232 control of an Acer H6512BD projector via a
MAX3232 level converter. Includes captive-portal WiFi provisioning and static-IP support.

## Technical Context

**Language/Version**: C/C++, Arduino-ESP32 framework (latest stable via PlatformIO)

**Primary Dependencies**:
- `tzapu/WiFiManager` — captive portal WiFi provisioning
- `me-no-dev/ESPAsyncWebServer` + `me-no-dev/AsyncTCP` — REST API + web UI
- `knolleary/PubSubClient` — MQTT client
- `bblanchon/ArduinoJson` v7 — JSON serialization (stack-allocated)
- `Preferences` (built-in) — NVS key-value storage

**Storage**: ESP32 NVS (non-volatile storage) via `Preferences` for config;
LittleFS for serving HTML/CSS/JS web UI assets

**Testing**: Hardware-in-the-loop against physical Acer H6512BD; optional native
host tests for RS232 command encoding logic

**Target Platform**: ESP32-S3 DevKitC-1 N16R8, PlatformIO `esp32-s3-devkitc-1` board

**Project Type**: Embedded firmware (single-binary)

**Performance Goals**: Boot-to-operational ≤ 30 s; command round-trip ≤ 3 s;
RS232 timeout threshold = 500 ms

**Constraints**: 16 MB Flash, 8 MB PSRAM; static buffers for RS232 framing;
watchdog enabled; no auth required; PlatformIO ONLY

**Scale/Scope**: Single device, single beamer, low concurrency (< 5 simultaneous clients)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. RS232-Serial Communication | ✅ PASS | All beamer control via MAX3232 + HardwareSerial; no alternative paths |
| II. WiFi Remote Control | ✅ PASS | WiFiManager + ESPAsyncWebServer + PubSubClient; auto-reconnect in both |
| III. Reliability-First | ✅ PASS | Watchdog enabled; 500 ms RS232 timeout → HTTP 503; boot ≤ 30 s targeted |
| IV. Embedded-Efficient Design | ✅ PASS | PlatformIO + Arduino-ESP32; ArduinoJson stack-alloc; static serial buffers |
| V. Simplicity & Minimal Footprint | ✅ PASS | Only required features; NVS config; no auth overhead; WiFiManager covers AP flow |
| Hardware: UART must be hardware | ✅ PASS | HardwareSerial on UART1/UART2; software serial prohibited |
| Build: PlatformIO ONLY | ✅ PASS | platformio.ini is the only build entrypoint |

*Post-Phase-1 re-check: All gates still pass. No violations.*

## Project Structure

### Documentation (this feature)

```text
specs/001-rs232-wifi-adapter/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   ├── rest-api.md      # REST API contract
│   └── mqtt-contract.md # MQTT topic contract
└── tasks.md             # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root)

```text
src/
├── main.cpp             # Entry point: setup() + loop()
├── BeamerRS232.h/.cpp   # RS232 command encoder + UART driver
├── BeamerStatus.h       # BeamerStatus struct + state cache
├── MqttManager.h/.cpp   # PubSubClient wrapper + topic routing
├── RestHandler.h/.cpp   # ESPAsyncWebServer route handlers
├── ConfigManager.h/.cpp # NVS Preferences wrapper (NetworkConfig, MqttConfig, Rs232Config)
├── WifiProvisioner.h/.cpp # WiFiManager setup + reset-button handler
└── WebUI.h/.cpp         # Serves LittleFS HTML/CSS/JS

data/                    # LittleFS filesystem (uploaded separately)
├── index.html           # Control web page
├── style.css
└── app.js

test/
└── test_rs232_encoder/  # Native host tests for RS232 command encoding

platformio.ini
README.md
.gitignore
```

**Structure Decision**: Single project, flat `src/` layout. Each module has a single
responsibility; no subfolders needed at this scale.

## Complexity Tracking

No constitution violations to justify.

## Phase 0: Research — COMPLETE

See [research.md](research.md).

**Resolved items**:
- Acer H6512BD RS232: 9600/8N1, `*0 IR 001\r` (power on), `*0 IR 002\r` (off),
  C36/C05/C33/C34/C35 (inputs), ACK=0x06 response — HiL verification needed for blank cmd
- PlatformIO libraries: WiFiManager, ESPAsyncWebServer, PubSubClient, ArduinoJson v7
- NVS config schema: 10 keys (see research.md)

## Phase 1: Design — COMPLETE

### data-model.md
See [data-model.md](data-model.md).
- 5 entities: BeamerStatus, BeamerCommand, NetworkConfig, MqttConfig, Rs232Config
- Boot state machine documented
- Input validation rules defined

### contracts/
See [contracts/rest-api.md](contracts/rest-api.md) and [contracts/mqtt-contract.md](contracts/mqtt-contract.md).
- REST: 5 endpoints (`GET /api/status`, `POST /api/power`, `POST /api/input`, `POST /api/blank`, `GET /api/config`, `GET /`)
- MQTT: 3 cmnd topics + 1 stat topic + LWT; Tasmota-style prefix/cmnd/stat

### quickstart.md
See [quickstart.md](quickstart.md).
- Build + flash via PlatformIO
- Captive portal WiFi setup
- REST/MQTT/Web validation steps
- Reset procedure

### README.md
See project root [README.md](../../README.md).
- Project overview + hardware wiring guide

## Next Step

Run `/speckit-tasks` to generate the dependency-ordered task list.
