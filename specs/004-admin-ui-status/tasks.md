# Tasks: Admin UI Status & Configuration Improvements

**Input**: Design documents from `specs/004-admin-ui-status/`

**Prerequisites**: plan.md ✅ spec.md ✅ research.md ✅ data-model.md ✅ contracts/ ✅ quickstart.md ✅

**Tests**: Not requested — no test tasks generated.

**Organization**: Tasks grouped by user story (US1–US5) for independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on other incomplete tasks)
- **[Story]**: User story label (US1–US5)

---

## Phase 1: Foundational (Blocking Prerequisite)

**Purpose**: `isConnected()` accessor must exist before `RestHandler` (US4) can call it.

**⚠️ CRITICAL**: Must complete before Phase 4 (US4) begins.

- [x] T001 Add `bool isConnected() const;` to `src/MqttManager.h` public interface and implement it returning `_client.connected()` in `src/MqttManager.cpp`

**Checkpoint**: `gMqtt.isConnected()` is callable from anywhere — US4 backend work can begin.

---

## Phase 2: User Story 1 — View Current Configuration Status (Priority: P1) 🎯 MVP

**Goal**: Admin page pre-fills every stored setting; unconfigured sections are visually distinct.

**Independent Test**: Open `/admin` after saving a Shelly IP; the IP field must be pre-filled. Sections with no data (MQTT unconfigured) must show a collapsed/hidden state.

- [x] T002 [P] [US1] Extend `GET /admin/config` handler in `src/AdminHandler.cpp` to include `ssid`, `staticIp`, `ipAddr`, `gateway`, `subnet`, `currentIp` (from `WiFi.localIP().toString()`) in the JSON response
- [x] T003 [P] [US1] Add `<span class="status-dot" id="dot-light"></span>` and `<span class="status-dot" id="dot-screen"></span>` next to each Shelly IP label in `data/admin.html`
- [x] T004 [P] [US1] Wrap all MQTT input fields in `<div id="mqtt-fields" class="hidden">` in `data/admin.html`; always show the MQTT enable toggle outside the wrapper
- [x] T005 [US1] Extend the `/admin/config` fetch block in `data/admin.html` JS to: populate `wifi_ssid` placeholder with current SSID, call `setDot('dot-light', !!d.lightIp)` and `setDot('dot-screen', !!d.screenIp)`, show `mqtt-fields` div when `d.mqttEnabled || d.mqttHost`; add `setDot(id, configured)` helper function

**Checkpoint**: Opening `/admin` shows all stored settings pre-filled; MQTT section hidden when broker host is empty; Shelly dots green when IP is set.

---

## Phase 3: User Story 2 — View and Edit WiFi & IP Settings (Priority: P1)

**Goal**: Admin page has an editable WiFi section (SSID, password, static IP) that saves changes on submit.

**Independent Test**: Enter a new SSID in the WiFi section, save → `gConfig.network.ssid` updates. Enable static IP, enter address → stored and shown on next page load.

- [x] T006 [US2] Add WiFi credentials card to `data/admin.html` (before the MQTT section): SSID text input `wifi_ssid`, masked password input `wifi_password` with "leer = unverändert" placeholder
- [x] T007 [US2] Add static IP toggle (`static_ip` checkbox) and collapsible address fields (`ip_addr`, `ip_gw`, `ip_sub`) inside the WiFi card in `data/admin.html`; add `toggleStaticIp(on)` JS helper that shows/hides `static-ip-fields` div
- [x] T008 [US2] Extend `/admin/config` fetch in `data/admin.html` JS to populate WiFi fields: set `static_ip` checkbox, call `toggleStaticIp(d.staticIp)`, pre-fill `ip_addr`/`ip_gw`/`ip_sub` when present
- [x] T009 [US2] Add `wifi_ssid`, `wifi_password`, `static_ip`, `ip_addr`, `ip_gw`, `ip_sub` parameter reading and `gConfig.network` update logic to `POST /admin/save` in `src/AdminHandler.cpp`; call `gConfig.saveNetwork(gConfig.network)` after changes; set `wifiChanged` flag
- [x] T010 [US2] Add WiFi restart banner trigger to `data/admin.html` JS: check `params.get('wifi') === '1'` and show `restart-banner` when true

**Checkpoint**: Change SSID in admin form, save → page redirects with `?wifi=1` → restart banner shown. Static IP toggle reveals address fields; toggling off hides them.

---

## Phase 4: User Story 3 — Partial Save (Priority: P2)

**Goal**: Submitting the admin form with empty fields leaves existing stored values unchanged.

**Independent Test**: Save admin form with only MQTT host changed, leaving Shelly IPs blank → Shelly IPs in stored config are identical to pre-save values.

- [x] T011 [US3] Change Shelly IP handling in `POST /admin/save` (`src/AdminHandler.cpp`): replace the current 400-on-invalid-IP block with `if (lightIp.length() > 0 && validIp(lightIp)) gConfig.shelly.lightIp = lightIp;` and same for `screenIp`; only reject non-empty invalid IPs
- [x] T012 [US3] Apply partial-save skip to MQTT fields in `POST /admin/save` (`src/AdminHandler.cpp`): only update `gConfig.mqtt.host` when `mqttHost.length() > 0`; only update port when `mqttPort.length() > 0`; only update prefix when `mqttPfx.length() > 0`; always process `mqtt_enabled` toggle
- [x] T013 [US3] Apply partial-save skip to credentials in `POST /admin/save` (`src/AdminHandler.cpp`): only update `adminPwHash` when `newPass.length() > 0`; only update `adminUser` when `newUser.length() > 0`
- [x] T014 [US3] Implement smart redirect in `POST /admin/save` (`src/AdminHandler.cpp`): redirect to `/admin?wifi=1` when `wifiChanged`, `/admin?mqtt=1` when MQTT changed, `/admin?wifi=1&mqtt=1` when both, `/admin` otherwise

**Checkpoint**: Save form with one field changed; inspect `/admin/config` response — all other fields match pre-save values.

---

## Phase 5: User Story 4 — IP Address and MQTT Status on Main Page (Priority: P2)

**Goal**: Main page status bar shows device IP and live MQTT connection state (polled every 5 s).

**Independent Test**: Load `/` → IP badge shows `192.168.178.249` (or current IP). With MQTT connected → "MQTT: ON" badge visible. Disconnect broker → badge disappears within one polling cycle.

- [x] T015 [P] [US4] Extend `_sendStatus()` in `src/RestHandler.cpp`: add `doc["ip"] = WiFi.localIP().toString();` and `doc["mqttConnected"] = gMqtt.isConnected();`; add `#include <ESP8266WiFi.h>` at top if absent; add `#include "MqttManager.h"` if absent
- [x] T016 [P] [US4] Add `<span id="status-ip" class="badge hidden">–</span>` and `<span id="status-mqtt" class="badge hidden">MQTT: OFF</span>` to `#status-bar` in `data/index.html`
- [x] T017 [US4] Extend `updateUI(status)` in `data/app.js`: if `status.ip` is present and not `"0.0.0.0"` update `#status-ip` text and remove `hidden` class; if `status.mqttConnected` is true show `#status-mqtt` with text "MQTT: ON", otherwise add `hidden` class

**Checkpoint**: Open `/` — IP badge visible in header. If MQTT configured and broker up → "MQTT: ON" badge. Wait one poll cycle after broker disconnect → badge hidden.

---

## Phase 6: User Story 5 — IP Address Published via MQTT (Priority: P3)

**Goal**: Device publishes its IP as a retained MQTT message each time it connects to the broker.

**Independent Test**: Subscribe to `<prefix>/ip` on the broker; within 5 s of device boot with MQTT enabled, a retained message with the IP string arrives.

- [x] T018 [US5] In `MqttManager::_reconnect()` in `src/MqttManager.cpp`, after successful connect + subscribe, add: `String ipTopic = _prefix + "/ip"; _client.publish(ipTopic.c_str(), WiFi.localIP().toString().c_str(), true);`

**Checkpoint**: MQTT broker shows retained message on `<prefix>/ip` after device connects.

---

## Phase 7: Polish & Deployment

**Purpose**: Build, flash, and verify end-to-end.

- [x] T019 Build and upload firmware: `pio run --target upload --environment d1_mini`
- [x] T020 [P] Build and upload filesystem: `pio run --target uploadfs --environment d1_mini`
- [ ] T021 Run quickstart.md verification checklist (IP badge, MQTT badge, admin pre-fill, MQTT retained topic, partial-save smoke test)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Foundational (Phase 1)**: No dependencies — start immediately
- **US1 (Phase 2)**: Depends on Phase 1 (T001) — can start after `isConnected()` exists (though US1 itself doesn't call it)
  - Actually US1 tasks T002–T005 do NOT depend on T001; they can start immediately in parallel with Phase 1
- **US2 (Phase 3)**: Depends on US1 Phase 2 completing (admin.html structure must be in place for T006–T010)
- **US3 (Phase 4)**: Depends on US2 (T009 must exist before partial-save refinements in T011–T014)
- **US4 (Phase 5)**: T015 depends on T001 (needs `isConnected()`). T016/T017 are independent.
- **US5 (Phase 6)**: Independent of all user stories except Phase 1 (same file as T001)
- **Polish (Phase 7)**: Depends on all desired story phases complete

### User Story Dependencies

- **US1 (P1)**: Can start immediately — no blocking dependencies
- **US2 (P1)**: Depends on US1 HTML structure (admin.html card layout) — start after Phase 2
- **US3 (P2)**: Depends on US2 save handler (`POST /admin/save` must have WiFi params before partial-save logic)
- **US4 (P2)**: T015 depends on T001; T016 and T017 are independent
- **US5 (P3)**: Independent; only dependency is `MqttManager.cpp` (same file as T001, sequence after)

### Parallel Opportunities

Within Phase 2: T002 (C++ file) runs in parallel with T003, T004 (HTML file).
Within Phase 5: T015 (C++ RestHandler) runs in parallel with T016 (HTML).
US5 (T018, MqttManager.cpp) can run in parallel with US4 (T015–T017, different files).

---

## Parallel Example: US1 + US5 concurrently

```
# These can run at the same time (different files):
T002: Extend /admin/config in src/AdminHandler.cpp
T003: Add Shelly dots to data/admin.html
T004: Add MQTT conditional wrapper to data/admin.html
T018: Publish retained IP in src/MqttManager.cpp
```

---

## Implementation Strategy

### MVP First (US1 + US4 deliver immediate value)

1. Complete Phase 1: T001 (Foundational)
2. Complete Phase 2: T002–T005 (US1 — admin pre-fill, all stored settings visible)
3. Complete Phase 5: T015–T017 (US4 — IP + MQTT badge on main page)
4. **STOP and VALIDATE**: Admin shows all config; main page shows IP; MQTT badge works
5. Deploy firmware + filesystem

### Incremental Delivery

1. Phase 1 + Phase 2 → Admin page shows stored config (US1)
2. Phase 3 → WiFi editing in admin (US2)
3. Phase 4 → Partial save (US3)
4. Phase 5 → IP + MQTT on main page (US4)
5. Phase 6 → MQTT IP publish (US5)
6. Phase 7 → Build, flash, verify

---

## Notes

- All C++ changes require firmware upload; all `data/` changes require filesystem upload
- T019 and T020 (upload steps) must run after all implementation phases complete
- [P] tasks touch different source files — safe to work in parallel
- Commit after each phase checkpoint to preserve incremental progress
