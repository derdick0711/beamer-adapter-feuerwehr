---
description: "Task list for RS232-WiFi Beamer Adapter implementation"
---

# Tasks: RS232-WiFi Beamer Adapter

**Input**: Design documents from `specs/001-rs232-wifi-adapter/`

**Prerequisites**: plan.md ✅ spec.md ✅ research.md ✅ data-model.md ✅ contracts/ ✅

**Tests**: HiL (Hardware-in-the-Loop) validation tasks included — no automated unit tests
unless explicitly requested. Each story has a dedicated HiL validation task.

**Organization**: Tasks grouped by user story for independent implementation and testing.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: PlatformIO project initialization and module skeleton

- [x] T001 Create PlatformIO project structure: `platformio.ini`, `src/`, `data/`, `test/` at repository root
- [x] T002 Configure `platformio.ini`: board `esp32-s3-devkitc-1`, framework `arduino`, filesystem `littlefs`, libraries (WiFiManager, ESPAsyncWebServer, AsyncTCP, PubSubClient, ArduinoJson)
- [x] T003 [P] Create `.gitignore`: exclude `.pio/`, `secrets.h`, any NVS dump files
- [x] T004 [P] Create empty skeleton header files in `src/`: `BeamerRS232.h`, `BeamerStatus.h`, `MqttManager.h`, `RestHandler.h`, `ConfigManager.h`, `WifiProvisioner.h`, `WebUI.h`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core modules that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T005 Implement `ConfigManager` in `src/ConfigManager.h` / `src/ConfigManager.cpp`: NVS `Preferences` wrapper for `NetworkConfig` (ssid, password, staticIp, ipAddr, gateway, subnet), `MqttConfig` (host, port, prefix), `Rs232Config` (baud=9600, rxPin=16, txPin=17); load/save/reset methods
- [x] T006 [P] Implement `BeamerStatus` in `src/BeamerStatus.h`: struct with fields `power` (on/off/unknown), `input` (hdmi/vga/component/svideo/composite/unknown), `blank` (bool), `reachable` (bool), `lastUpdated` (uint32); global singleton with update method
- [x] T007 [P] Implement `BeamerRS232` in `src/BeamerRS232.h` / `src/BeamerRS232.cpp`: init `HardwareSerial` on UART1 (pins from `Rs232Config`), command encoder (power/input/blank → RS232 byte strings per research.md), `sendCommand()` with 500 ms ACK/NAK timeout, returns `bool reachable`
- [x] T008 Implement `WifiProvisioner` in `src/WifiProvisioner.h` / `src/WifiProvisioner.cpp`: `WiFiManager` setup, AP name `BeamerAdapter-Setup`, custom param placeholders (static IP, MQTT), `begin()` blocks until connected, exposes `isConfigured()` and `resetConfig()` methods
- [x] T009 Wire `main.cpp` `setup()` boot sequence: `ConfigManager::load()` → `WifiProvisioner::begin()` → start UART (`BeamerRS232::init()`) → start HTTP server → start MQTT client; enable hardware watchdog; `loop()` handles MQTT keepalive + reset-button poll

**Checkpoint**: Foundation ready — all user story phases can now begin

---

## Phase 3: User Story 4 — WiFi AP Provisioning (Priority: P1) 🎯 MVP

**Goal**: Fresh or reset adapter opens captive-portal AP, accepts WLAN + static IP + MQTT
config, saves to NVS, and reconnects automatically.

**Independent Test**: Flash a factory-reset device → AP `BeamerAdapter-Setup` appears →
configure WLAN via browser → adapter restarts and connects; hold BOOT button 5 s → AP
reappears.

- [x] T010 [P] [US4] Add static IP custom parameters to WiFiManager portal in `src/WifiProvisioner.cpp`: fields `ip_addr`, `ip_gw`, `ip_subnet`, `ip_static` (checkbox); save to `ConfigManager` on portal save callback
- [x] T011 [P] [US4] Add MQTT broker custom parameters to WiFiManager portal: fields `mqtt_host`, `mqtt_port`, `mqtt_prefix`; save to `ConfigManager` on portal save callback
- [x] T012 [US4] Implement 5-minute AP-mode timeout in `src/WifiProvisioner.cpp`: if no config saved within 300 s, call `ESP.restart()`; re-enters AP mode on next boot
- [x] T013 [US4] Implement reset-button 5 s hold in `main.cpp` `loop()`: poll GPIO0 LOW continuously; on 5 s threshold call `WifiProvisioner::resetConfig()` (clears NVS WLAN keys) + `ESP.restart()`
- [x] T014 [US4] Implement static IP application in `WifiProvisioner::begin()`: if `NetworkConfig.staticIp == true`, call `WiFi.config(ip, gw, subnet)` before `WiFiManager::autoConnect()`
- [ ] T015 [US4] HiL test: (1) fresh flash → AP visible → configure → reconnects; (2) static IP set → verify with `ip addr` on router; (3) BOOT button 5 s → AP reappears; (4) AP timeout 5 min → auto-restart

**Checkpoint**: User Story 4 complete — WiFi provisioning fully functional and independently testable

---

## Phase 4: User Story 1 — REST API Beamer Control (Priority: P1) 🎯 MVP

**Goal**: All beamer functions (power, input, blank, status) accessible via HTTP REST.

**Independent Test**: `curl http://<ip>/api/status` returns JSON; `POST /api/power {"state":"on"}`
switches beamer on and returns 200; `POST /api/input {"input":"vga"}` changes source;
`POST /api/blank {"enabled":true}` blacks out image — all verified on physical Acer H6512BD.

- [x] T016 [P] [US1] Implement `GET /api/status` in `src/RestHandler.cpp`: return `BeamerStatus` as JSON; if `reachable==false`, include `lastKnown` and return HTTP 200 (status is informational, not an error)
- [x] T017 [P] [US1] Implement `POST /api/power` in `src/RestHandler.cpp`: parse `{"state":"on"/"off"}`, call `BeamerRS232::sendCommand()`, update `BeamerStatus`, return 200 or 503 on timeout
- [x] T018 [P] [US1] Implement `POST /api/input` in `src/RestHandler.cpp`: validate input value against allowed list (hdmi/vga/component/svideo/composite), call RS232 command, return 200/400/503
- [x] T019 [P] [US1] Implement `POST /api/blank` in `src/RestHandler.cpp`: parse `{"enabled":true/false}`, send RS232 blank command, return 200/400/503
- [x] T020 [US1] Implement `GET /api/config` in `src/RestHandler.cpp`: return non-secret config (staticIp, mqttHost, mqttPort, mqttPrefix) as JSON
- [x] T021 [US1] Add `RestHandler::begin()` in `src/RestHandler.cpp`: register all routes on `AsyncWebServer`; add 400 JSON error handler for missing/invalid fields; add 503 handler for RS232 timeout; call from `main.cpp` `setup()`
- [ ] T022 [US1] HiL test: `curl` all REST endpoints against physical Acer H6512BD — power on, all 5 inputs, blank on/off, status; verify 503 with beamer RS232 cable unplugged; verify 400 with invalid payload

**Checkpoint**: User Story 1 complete — beamer fully controllable via REST independently of MQTT/Web

---

## Phase 5: User Story 2 — MQTT Beamer Control (Priority: P2)

**Goal**: All beamer functions controllable via MQTT; status published after each command;
LWT on disconnect.

**Independent Test**: `mosquitto_pub -t beamer/cmnd/power -m on` switches beamer on;
`mosquitto_sub -t beamer/stat` shows updated status JSON; disconnect adapter → LWT
appears on `beamer/stat`.

- [x] T023 [P] [US2] Implement `MqttManager` connect in `src/MqttManager.cpp`: `PubSubClient` init with broker host/port from `MqttConfig`; set LWT (`{prefix}/stat`, `{"reachable":false,"reason":"lwt"}`, retain=true); `connect()` with client ID `beamer-adapter-{MAC}`
- [x] T024 [P] [US2] Implement MQTT auto-reconnect in `src/MqttManager.cpp`: `reconnect()` with exponential back-off (max 60 s); call in `loop()` if `!client.connected()`
- [x] T025 [P] [US2] Implement MQTT subscribe + message callback in `src/MqttManager.cpp`: subscribe to `{prefix}/cmnd/power`, `{prefix}/cmnd/input`, `{prefix}/cmnd/blank`; dispatch to `BeamerRS232::sendCommand()` per payload
- [x] T026 [US2] Implement `MqttManager::publishStatus()` in `src/MqttManager.cpp`: serialize `BeamerStatus` to JSON, publish to `{prefix}/stat` with retain=true; call after every command execution (REST and MQTT) and on boot
- [x] T027 [US2] Wire `MqttManager::begin()` call in `main.cpp` `setup()` (after WLAN connected); add `MqttManager::loop()` to `main.cpp` `loop()`
- [ ] T028 [US2] HiL test: `mosquitto_pub` all three cmnd topics → verify beamer responds and `beamer/stat` updates; cut power to adapter → verify LWT on broker; restart adapter → verify auto-reconnect

**Checkpoint**: User Story 2 complete — MQTT fully functional and independently testable

---

## Phase 6: User Story 3 — Web UI (Priority: P2)

**Goal**: Browser-accessible control page on adapter IP with all beamer functions and
live status display; works on smartphone without additional software.

**Independent Test**: Open `http://<adapter-ip>/` on smartphone browser → page loads →
press "Einschalten" → beamer turns on and status updates; select input dropdown → beamer
switches input.

- [x] T029 [P] [US3] Create `data/index.html`: buttons for Power On/Off, input selector (`<select>` with all 5 options), Blank On/Off toggle, status display section (power state, input, blank, reachable)
- [x] T030 [P] [US3] Create `data/style.css`: mobile-first responsive layout; large touch targets (min 48px); readable on small screens without zoom
- [x] T031 [US3] Create `data/app.js`: `fetch()` calls to REST API for each button/select action; `GET /api/status` polling every 5 s; update DOM with current status values; show error toast on 503
- [x] T032 [US3] Implement `WebUI::begin()` in `src/WebUI.cpp`: mount LittleFS; register `GET /` route on `AsyncWebServer` to serve `data/index.html` from LittleFS; call from `main.cpp` `setup()`
- [ ] T033 [US3] Flash LittleFS filesystem: `pio run -t uploadfs`; verify `data/index.html` served at `http://<ip>/`
- [ ] T034 [US3] HiL test: open web UI on smartphone (Chrome/Firefox/Safari) — all controls work; status auto-refreshes; test on desktop browser as well

**Checkpoint**: All user stories complete and independently functional

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Reliability, build hardening, documentation validation

- [x] T035 [P] Verify hardware watchdog enabled in `main.cpp`: `esp_task_wdt_init()` with 10 s timeout; add `esp_task_wdt_reset()` in main `loop()`
- [x] T036 [P] Add `LOG_LEVEL` compile flag in `platformio.ini`: `build_flags = -DLOG_LEVEL=0` for release; document in README.md how to enable verbose logging for debug
- [ ] T037 HiL timing test: power-cycle adapter → measure time to first successful `/api/status` response; MUST be ≤ 30 s per constitution
- [ ] T038 [P] HiL verify open items from `research.md`: test Blank command (`*0 IR 055\r`) behaviour (toggle or separate on/off); test Composite input code (C35); test status query format; update `research.md` with confirmed values
- [ ] T039 [P] Review and validate `README.md` hardware wiring section against actual MAX3232 board pin labels; update if needed

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 — BLOCKS all user stories
- **US4 (Phase 3)**: Depends on Phase 2 — WiFi provisioning enables all other stories
- **US1 (Phase 4)**: Depends on Phase 2 (and practically Phase 3 for WiFi) — primary MVP
- **US2 (Phase 5)**: Depends on Phase 2; integrates with US1's `BeamerRS232` module
- **US3 (Phase 6)**: Depends on Phase 4 (REST API must exist to call from JS)
- **Polish (Phase 7)**: Depends on all stories complete

### User Story Dependencies

- **US4 (P1)**: Foundational done — no story dependencies
- **US1 (P1)**: Foundational done — no story dependencies (WiFi assumed already connected)
- **US2 (P2)**: US1 done recommended (shares `BeamerRS232` + `BeamerStatus`)
- **US3 (P2)**: US1 done required (Web UI calls REST API)

### Within Each User Story

- Models/structs before services (T006 before T007 before T023)
- `begin()` wiring in main.cpp last within each story
- HiL test is always the final task of each story phase

### Parallel Opportunities

- T003, T004 parallel with T002 (all Phase 1 setup)
- T006, T007 parallel with T005 (independent structs)
- T010, T011 parallel within US4
- T016, T017, T018, T019 parallel within US1 (different routes, different files)
- T023, T024, T025 parallel within US2
- T029, T030 parallel within US3

---

## Parallel Example: US1

```
# Launch all REST route handlers in parallel (independent methods in RestHandler.cpp):
Task T016: Implement GET /api/status
Task T017: Implement POST /api/power
Task T018: Implement POST /api/input
Task T019: Implement POST /api/blank
# Then sequentially:
Task T020: GET /api/config
Task T021: Wire all routes + error handlers
Task T022: HiL validation
```

---

## Implementation Strategy

### MVP First (US4 + US1 only)

1. Phase 1: Setup
2. Phase 2: Foundational
3. Phase 3: US4 — WiFi provisioning
4. Phase 4: US1 — REST API
5. **STOP and VALIDATE**: Full HiL test of REST API on physical Acer H6512BD
6. Deploy to fire department environment for field testing

### Incremental Delivery

1. Setup + Foundational → Core modules ready
2. US4 → Adapter configurable in network
3. US1 → Beamer controllable via REST (MVP!)
4. US2 → MQTT integration for automation systems
5. US3 → Web UI for direct browser control
6. Polish → Production hardening

---

## Notes

- `[P]` = task operates on different files from peers, no blocking dependency
- `[US1/2/3/4]` = maps task to user story for traceability
- HiL tests always last in each story — do not skip
- Open RS232 items (blank cmd, composite code, status query) → resolved in T038
- WiFi credentials MUST NOT be in source; use NVS provisioning only
