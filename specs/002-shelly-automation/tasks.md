---
description: "Task list for Shelly-Integration & Präsentationsautomatisierung"
---

# Tasks: Shelly-Integration & Präsentationsautomatisierung

**Input**: Design documents from `specs/002-shelly-automation/`

**Prerequisites**: plan.md ✅ spec.md ✅ research.md ✅ data-model.md ✅ contracts/ ✅

**Tests**: HiL (Hardware-in-the-Loop) validation tasks included — no automated unit tests
unless explicitly requested. Each story has a dedicated HiL validation task.

**Organization**: Tasks grouped by user story for independent implementation and testing.
Feature 001 infrastructure (ConfigManager, RestHandler, MqttManager, main.cpp) is extended
additively — no existing code is replaced.

**Devices**:
- Shelly 1 Mini Gen3 → Deckenlicht (Switch, component id=0)
- Shelly 2PM Gen3 → Leinwand (Cover/Roller, component id=0, muss vorher im Roller-Modus konfiguriert werden)

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Skeleton-Dateien anlegen; keine neue Library erforderlich
(`HTTPClient` und `mbedTLS/SHA-256` sind im Arduino-ESP32-Core bereits enthalten)

- [x] T001 [P] Create skeleton header files: `src/ShellyClient.h`, `src/SceneManager.h`, `src/AdminHandler.h`
- [x] T002 [P] Verify `platformio.ini` — no new lib_deps required; `HTTPClient` and `mbedtls` are Arduino-ESP32 built-ins

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Kernmodule, die ALLE User Stories voraussetzen

**⚠️ CRITICAL**: Kein User-Story-Code darf beginnen, bevor diese Phase abgeschlossen ist

- [x] T003 Extend `src/ConfigManager.h` / `src/ConfigManager.cpp`: Add `ShellyConfig` struct with fields `lightIp` (String), `screenIp` (String), `adminPwHash` (String); NVS namespace `shelly`; keys `light_ip`, `screen_ip`, `admin_pw`; defaults `192.168.1.100`, `192.168.1.101`, SHA-256(`"feuerwehr"`); add `loadShelly()` / `saveShelly()` methods; ShellyConfig added as `gConfig.shelly` field
- [x] T004 [P] Implement `ShellyClient` in `src/ShellyClient.h` / `src/ShellyClient.cpp`: `HTTPClient`-based; `setSwitch(ip, on)` → `GET /rpc/Switch.Set?id=0&on=true/false`; `getSwitchStatus(ip)` → `GET /rpc/Switch.GetStatus?id=0`; `coverOpen/Close/Stop(ip)` → `GET /rpc/Cover.Open|Close|Stop?id=0`; `coverGoToPosition(ip, pos)` → `GET /rpc/Cover.GoToPosition?id=0&pos={pos}`; `getCoverStatus(ip)` → `GET /rpc/Cover.GetStatus?id=0`; all methods: 3000 ms timeout; return struct `{bool success, String error}`; update `gLight` or `gScreen` singleton on success
- [x] T005 [P] Define `ShellySwitchDevice` and `ShellyRollerDevice` status structs in `src/ShellyClient.h` per data-model.md; declare `extern ShellySwitchDevice gLight` and `extern ShellyRollerDevice gScreen` singletons; implement in `src/ShellyClient.cpp` with default values (ip from gShellyConfig, reachable=false, output=false, state="unknown", currentPos=0)
- [x] T006 Wire `ConfigManager::loadShelly()` into `main.cpp` `setup()` after `gConfig.load()` to populate `gShellyConfig`; init `gLight.ip = gShellyConfig.lightIp` and `gScreen.ip = gShellyConfig.screenIp`

**Checkpoint**: Foundation ready — ShellyClient kann gegen physische Geräte getestet werden

---

## Phase 3: User Story 1 — Präsentation starten (Priority: P1) 🎯 MVP

**Goal**: Ein Tastendruck startet die vollständige Präsentationssequenz:
Leinwand runter → Licht aus → Beamer ein → HDMI-Eingang

**Independent Test**: `curl -X POST http://<ip>/api/scene/start` → Response enthält alle 4 Steps
mit `success=true`; Sequenz dauert < 15 s; Leinwand fährt, Licht geht aus, Beamer startet,
HDMI aktiv. Auch via MQTT: `mosquitto_pub -t beamer/cmnd/scene/start -m "1"`.

- [x] T007 [P] [US1] Define `SceneStep` and `SceneResult` structs in `src/SceneManager.h` per data-model.md
- [x] T008 [US1] Implement `SceneManager::runStart()` in `src/SceneManager.cpp`: (1) `ShellyClient::coverClose(gScreen.ip)`, (2) `ShellyClient::setSwitch(gLight.ip, false)`, (3) `BeamerRS232::sendCommand(power_on)`, (4) `BeamerRS232::sendCommand(input_hdmi)`; each step records `SceneStep{device, action, success, error}`; all steps run regardless of individual failures; returns `SceneResult{scene="start", success, steps, duration_ms}`
- [x] T009 [P] [US1] Implement `POST /api/scene/start` in `src/RestHandler.cpp`: call `SceneManager::runStart()`, serialize `SceneResult` to JSON with ArduinoJson v7, return HTTP 200 (always); register route in `RestHandler::begin()`
- [x] T010 [P] [US1] Subscribe to `{prefix}/cmnd/scene/start` in `src/MqttManager.cpp` `_onMessage` callback: call `SceneManager::runStart()`, publish `SceneResult` JSON to `{prefix}/stat/scene` (retain=false); add subscription in `MqttManager::begin()`
- [x] T011 [US1] Add "Präsentation starten" button (grün, volle Breite) to `data/index.html` Szenen-Sektion; add `startScene()` function to `data/app.js` using `apiFetch('POST', '/api/scene/start')` with loading state and result toast
- [ ] T012 [US1] HiL test: (1) POST `/api/scene/start` → alle 4 Schritte `success=true`, Gesamtdauer ≤ 15 s; (2) MQTT trigger `beamer/cmnd/scene/start` → `beamer/stat/scene` mit SceneResult; (3) Web-Button → Sequenz läuft; (4) Shelly 2PM Gen3 nicht erreichbar → restliche Schritte trotzdem ausgeführt, `success=false`

**Checkpoint**: User Story 1 complete — Szene "Präsentation starten" über alle 3 Kanäle auslösbar

---

## Phase 4: User Story 2 — Präsentation beenden (Priority: P1)

**Goal**: Ein Tastendruck beendet die Präsentation:
Leinwand hoch → Licht an → Beamer aus

**Independent Test**: `curl -X POST http://<ip>/api/scene/stop` → 3 Steps; Sequenz < 15 s;
Leinwand fährt hoch, Licht geht an, Beamer schaltet aus.

- [x] T013 [US2] Implement `SceneManager::runStop()` in `src/SceneManager.cpp`: (1) `ShellyClient::coverOpen(gScreen.ip)`, (2) `ShellyClient::setSwitch(gLight.ip, true)`, (3) `BeamerRS232::sendCommand(power_off)`; returns `SceneResult{scene="stop", ...}` — gleiche Teilfehler-Logik wie runStart()
- [x] T014 [P] [US2] Implement `POST /api/scene/stop` in `src/RestHandler.cpp`: call `SceneManager::runStop()`, return SceneResult JSON; register route in `RestHandler::begin()`
- [x] T015 [P] [US2] Subscribe to `{prefix}/cmnd/scene/stop` in `src/MqttManager.cpp`: call `SceneManager::runStop()`, publish to `{prefix}/stat/scene`; add subscription in `begin()`
- [x] T016 [US2] Add "Präsentation beenden" button (rot) to `data/index.html` Szenen-Sektion neben "starten"; add `stopScene()` to `data/app.js`
- [ ] T017 [US2] HiL test: POST `/api/scene/stop` → 3 Steps `success=true`, ≤ 15 s; MQTT trigger; Web-Button; Teilfehler bei ausgefallenem Gerät

**Checkpoint**: User Stories 1 + 2 complete — beide Szenen über alle Kanäle verfügbar

---

## Phase 5: User Story 3 — Deckenlicht manuell steuern (Priority: P2)

**Goal**: Deckenlicht unabhängig ein- und ausschalten via Web, REST, MQTT

**Independent Test**: `curl -X POST /api/light -d '{"state":"on"}'` → Licht geht an;
`curl /api/light/status` → `{"output":true,"reachable":true}`; MQTT `beamer/cmnd/light on`.

- [x] T018 [P] [US3] Implement `GET /api/light/status` in `src/RestHandler.cpp`: call `ShellyClient::getSwitchStatus(gLight.ip)`, update `gLight`, return `{"output":bool,"reachable":bool}` JSON; register route in `begin()`
- [x] T019 [P] [US3] Implement `POST /api/light` in `src/RestHandler.cpp`: parse `{"state":"on"/"off"}`, call `ShellyClient::setSwitch()`, return 200/400/503 per contracts/rest-api-additions.md; register route in `begin()`
- [x] T020 [P] [US3] Subscribe to `{prefix}/cmnd/light` in `src/MqttManager.cpp`: parse `on`/`off` payload, call `ShellyClient::setSwitch()`, publish `gLight` status to `{prefix}/stat/light` (retain=true); add subscription in `begin()`
- [x] T021 [US3] Add "Licht" Sektion zu `data/index.html`: Buttons "Licht ein" (gelb) + "Licht aus" (grau) + Status-Badge; add `setLight(state)` zu `data/app.js`; poll `GET /api/light/status` alle 5 s und DOM aktualisieren
- [ ] T022 [US3] HiL test: Licht ein/aus via Web-Button; REST POST; MQTT; `/api/light/status` gibt korrekten Zustand zurück; Shelly nicht erreichbar → 503

**Checkpoint**: User Story 3 complete — Lichtsteuerung unabhängig von Szenen nutzbar

---

## Phase 6: User Story 4 — Leinwand manuell steuern (Priority: P2)

**Goal**: Leinwand auf/ab/stop und Positionierung via Web, REST, MQTT

**Independent Test**: `curl -X POST /api/screen -d '{"action":"close"}'` → Leinwand fährt;
`curl -X POST /api/screen -d '{"action":"stop"}'` → stoppt sofort; Position 50% erreichbar.

- [x] T023 [P] [US4] Implement `GET /api/screen/status` in `src/RestHandler.cpp`: call `ShellyClient::getCoverStatus(gScreen.ip)`, update `gScreen`, return `{"state":str,"current_pos":int,"reachable":bool}` JSON; register route in `begin()`
- [x] T024 [P] [US4] Implement `POST /api/screen` in `src/RestHandler.cpp`: parse `{"action":"open"/"close"/"stop"/"position", "pos":0-100}`; dispatch to `ShellyClient::coverOpen/Close/Stop/GoToPosition`; return 200/400/503 per contracts; register in `begin()`
- [x] T025 [P] [US4] Subscribe to `{prefix}/cmnd/screen` in `src/MqttManager.cpp`: parse `open`/`close`/`stop`/`pos:{N}` payloads; dispatch to ShellyClient; publish `gScreen` status to `{prefix}/stat/screen` (retain=true); add subscription in `begin()`
- [x] T026 [US4] Add "Leinwand" Sektion zu `data/index.html`: Buttons "Runter", "Stop", "Hoch" + Position-Anzeige; add `setScreen(action, pos)` zu `data/app.js`; poll `GET /api/screen/status` alle 5 s
- [ ] T027 [US4] HiL test: Leinwand hoch/runter/stop via Web; REST open/close/stop/position=50; MQTT open/close/stop/pos:50; Status zeigt current_pos korrekt

**Checkpoint**: User Story 4 complete — manuelle Leinwandsteuerung unabhängig nutzbar

---

## Phase 7: User Story 5 — Admin-Oberfläche (Priority: P2)

**Goal**: Passwortgeschützte Admin-Seite zum Konfigurieren der Shelly-IPs und des Passworts;
Änderungen sofort wirksam ohne Neustart

**Independent Test**: Browser auf `/admin` → Login-Dialog → nach Eingabe von `feuerwehr` →
Konfigurationsseite erscheint mit aktuellen IPs; neue IP eintragen + speichern → Adapter
verwendet neue IP sofort; falsches Passwort → 401.

- [x] T028 [US5] Implement `AdminHandler` in `src/AdminHandler.h` / `src/AdminHandler.cpp`: SHA-256-Vergleich mit `mbedtls_md` (built-in); `_checkAuth(request)` prüft HTTP Basic Auth Password gegen `gShellyConfig.adminPwHash`; `begin(AsyncWebServer&)` registriert `GET /admin` (serveStatic aus LittleFS) + `POST /admin/save` (Formular-Parsing, IP-Validierung via `WiFi.hostByName()` oder Regex, NVS-Speicherung via `ConfigManager::saveShelly()`, live-Update von `gLight.ip` + `gScreen.ip`); Redirect 303 zu `/admin` nach Speicherung; `requestAuthentication()` bei fehlendem Auth
- [x] T029 [P] [US5] Create `data/admin.html`: HTML-Formular mit Feldern `light_ip` (Shelly 1 Mini Gen3 IP), `screen_ip` (Shelly 2PM Gen3 IP), `new_password` (optional); zeigt aktuelle Werte via `value=`-Platzhalter; Submit-Button "Speichern"; Link zurück zur Hauptseite; minimales CSS (kein externes Framework)
- [x] T030 [US5] Wire `AdminHandler::begin(server)` in `src/main.cpp` `setup()` nach `RestHandler::begin()`
- [ ] T031 [US5] HiL test: (1) `/admin` ohne Auth → 401 Basic-Auth-Dialog; (2) falsches Passwort → 401; (3) korrektes Passwort → Seite mit aktuellen IPs; (4) IP ändern + speichern → Redirect zurück, neue IP sofort in REST/MQTT aktiv (test mit `/api/light/status` auf neue IP); (5) Passwort ändern → altes Passwort funktioniert nicht mehr

**Checkpoint**: Alle User Stories complete — vollständige Feature-002-Implementierung

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Dokumentation, Timing-Validierung, README-Update

- [x] T032 Update `README.md`: Neue REST-Endpunkte (Shelly-Steuerung, Szenen, Admin), neue MQTT-Topics (light/screen/scene), Web-UI-Erweiterungen (Szenen-/Licht-/Leinwand-Karten), Admin-Oberfläche Beschreibung
- [ ] T033 HiL timing test: Messung Gesamtdauer scene/start mit beiden Shellies + Beamer → MUSS ≤ 15 s (SC-001); scene/stop → ≤ 15 s (SC-002); beide ohne Watchdog-Reset (Watchdog ist 10 s, aber Sequenz dauert ~2–3 s → kein Problem)
- [ ] T034 HiL verify Admin IP-Konfiguration: Zeit von Seitenöffnung bis Speicherung + Wirksamkeit < 2 Minuten (SC-005)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: Keine Abhängigkeiten — sofort starten
- **Foundational (Phase 2)**: Hängt von Phase 1 ab — BLOCKIERT alle User Stories
- **US1 (Phase 3)**: Hängt von Phase 2 ab — kein Beamer ohne ShellyClient
- **US2 (Phase 4)**: Hängt von Phase 2 ab; SceneManager::runStop() baut auf runStart()-Pattern auf (aber unabhängig testbar)
- **US3 (Phase 5)**: Hängt von Phase 2 ab — unabhängig von US1/US2
- **US4 (Phase 6)**: Hängt von Phase 2 ab — unabhängig von US1/US2/US3
- **US5 (Phase 7)**: Hängt von Phase 2 ab (ConfigManager::saveShelly()); unabhängig von US1–US4
- **Polish (Phase 8)**: Hängt von allen abgeschlossenen User Stories ab

### User Story Dependencies

- **US1 (P1)**: Foundational done — SceneManager + ShellyClient + RS232 ready
- **US2 (P1)**: Foundational done; nutzt `SceneManager` der in US1 erstellt wurde (T007 muss vor T013 kommen)
- **US3 (P2)**: Foundational done — nutzt nur ShellyClient (kein US1/US2 required)
- **US4 (P2)**: Foundational done — nutzt nur ShellyClient (kein US1/US2 required)
- **US5 (P2)**: Foundational done — nutzt nur ConfigManager (keine anderen US required)

### Within Each User Story

- SceneManager-Structs (T007) vor SceneManager-Implementierung (T008)
- ShellyClient-Calls in SceneManager vor REST/MQTT-Routing
- REST-Endpunkt vor Web-UI (JS ruft REST auf)
- HiL-Test immer letzter Task jeder Story-Phase

### Parallel Opportunities

- T001, T002 parallel (Phase 1)
- T003, T004, T005 parallel (Phase 2 — verschiedene Dateien)
- T007 parallel mit T009, T010 (structs unabhängig von Routing und MQTT)
- T018, T019, T020 parallel (US3 — verschiedene Handler, verschiedene Dateien)
- T023, T024, T025 parallel (US4 — verschiedene Handler)
- T028, T029 parallel (US5 — HTML und AdminHandler unabhängig)

---

## Parallel Example: US3

```
# Alle US3-Implementierungen gleichzeitig (verschiedene Dateien):
Task T018: GET /api/light/status  →  src/RestHandler.cpp (neue Route)
Task T019: POST /api/light         →  src/RestHandler.cpp (neue Route)
Task T020: MQTT cmnd/light         →  src/MqttManager.cpp (neues Subscribe)
# Dann sequenziell:
Task T021: data/index.html + app.js  (nutzt REST-Endpunkte von T018/T019)
Task T022: HiL test
```

---

## Implementation Strategy

### MVP First (US1 + US2 only)

1. Phase 1: Setup
2. Phase 2: Foundational (ConfigManager + ShellyClient)
3. Phase 3: US1 — Präsentation starten
4. Phase 4: US2 — Präsentation beenden
5. **STOP and VALIDATE**: Beide Szenen gegen physische Shelly Gen3 + Acer H6512BD testen
6. Bei Erfolg: weiter mit US3–US5

### Incremental Delivery

1. Setup + Foundational → ShellyClient operational
2. US1 → Szene "starten" funktioniert (MVP!)
3. US2 → Szene "beenden" funktioniert (MVP complete)
4. US3 → Lichtsteuerung unabhängig
5. US4 → Leinwandsteuerung unabhängig
6. US5 → Admin-Konfiguration
7. Polish → Timing-Validierung + Dokumentation

---

## Notes

- `[P]` = Task operiert auf anderen Dateien als parallele Peers, keine blockierende Abhängigkeit
- `[US1–US5]` = User-Story-Zuordnung für Traceability
- HiL-Tests immer am Ende jeder Story-Phase — nicht überspringen
- `HTTPClient` und `mbedtls` sind Arduino-ESP32-built-ins — kein `platformio.ini` Update nötig
- Shelly 2PM Gen3 muss vor HiL-Tests in Roller/Cover-Modus konfiguriert sein (via Shelly App)
- Passwort-Default `feuerwehr` — vor Produktivbetrieb über Admin-UI ändern
