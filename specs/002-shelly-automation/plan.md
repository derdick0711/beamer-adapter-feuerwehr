# Implementation Plan: Shelly-Integration & Präsentationsautomatisierung

**Branch**: `002-shelly-automation` | **Date**: 2026-05-23 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/002-shelly-automation/spec.md`

## Summary

Erweiterung des bestehenden ESP32-S3 Beamer-Adapters um HTTP-basierte Steuerung von zwei
Shelly Gen3 Geräten: Shelly 1 Mini Gen3 (Deckenlicht, Switch) und Shelly 2PM Gen3 (Leinwand,
Cover/Roller). Zwei Ein-Knopf-Automatisierungen orchestrieren beide Shellies und den Beamer
via RS232. Neue REST- und MQTT-Endpunkte sowie eine passwortgeschützte Admin-Oberfläche zur
IP-Konfiguration. Die gesamte Implementierung erweitert das bestehende PlatformIO-Projekt
aus Feature 001 ohne Umbau der bestehenden Architektur.

## Technical Context

**Language/Version**: C/C++, Arduino-ESP32 framework (wie Feature 001, via PlatformIO)

**Primary Dependencies**:
- `HTTPClient` (Arduino-ESP32 built-in) — synchrone HTTP GET-Anfragen an Shelly Gen3 RPC API
- `ESPAsyncWebServer` (bereits vorhanden) — neue REST-Routen + Admin-HTML-Seite
- `PubSubClient` (bereits vorhanden) — neue MQTT cmnd/stat Topics
- `ArduinoJson v7` (bereits vorhanden) — JSON für SceneResult-Serialisierung
- `Preferences` (bereits vorhanden) — NVS-Speicherung von Shelly-IPs und Admin-Passwort-Hash

**Storage**: NVS Namespace `shelly` für `light_ip`, `screen_ip`, `admin_pw` (SHA-256-Hash)

**Testing**: Hardware-in-the-Loop gegen physische Shelly Gen3 Geräte und Acer H6512BD

**Target Platform**: ESP32-S3 DevKitC-1 N16R8 (wie Feature 001)

**Project Type**: Embedded firmware — Erweiterung des Feature-001-Projekts

**Performance Goals**:
- Sequenz "Präsentation starten" (4 Schritte): ≤ 15 s (SC-001)
- Sequenz "Präsentation beenden" (3 Schritte): ≤ 15 s (SC-002)
- IP-Konfiguration via Admin-UI: ≤ 2 min (SC-005)
- Shelly HTTP-Request-Timeout: 3 s pro Request

**Constraints**: Keine neuen Library-Dependencies außer built-in `HTTPClient`;
gleiche ESP32-S3-Ressourcenbeschränkungen wie Feature 001; PlatformIO ONLY

**Scale/Scope**: 2 Shelly-Geräte, 2 Szenen, 4 manuelle Steuerungsendpunkte, 1 Admin-UI

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. RS232-Serial Communication | ✅ PASS | Beamer-Befehle in Szenen gehen weiterhin über MAX3232 RS232; keine Alternative eingeführt |
| II. WiFi Remote Control | ✅ PASS | Alle Shelly-Steuerung via WiFi (HTTP REST auf Shelly-IP); ESP32 bleibt WiFi-Client |
| III. Reliability-First (Feuerwehr-Grade) | ✅ PASS | Teilfehler-Handling (FR-009): Sequenz läuft durch; `reachable=false` → Fehler gemeldet; Watchdog unverändert aktiv |
| IV. Embedded-Efficient Design | ✅ PASS | `HTTPClient` ist built-in; keine neuen heap-intensiven Libraries; Stack-alloc ArduinoJson weiterhin genutzt |
| V. Simplicity & Minimal Footprint | ✅ PASS | Jede neue Funktion hat konkreten Feuerwehr-Anwendungsfall; Admin-UI erfüllt Konfigurierbarkeits-Anforderung |
| Hardware: UART must be hardware | ✅ PASS | Keine Änderung am RS232-Pfad |
| Build: PlatformIO ONLY | ✅ PASS | Keine neuen Build-System-Abhängigkeiten; `HTTPClient` ist Teil des Arduino-ESP32-Cores |

*Post-Phase-1 re-check: Alle Gates bestehen. Keine Violations.*

## Project Structure

### Documentation (this feature)

```text
specs/002-shelly-automation/
├── plan.md                         # This file
├── research.md                     # Phase 0 output
├── data-model.md                   # Phase 1 output
├── quickstart.md                   # Phase 1 output
├── contracts/
│   ├── rest-api-additions.md       # Neue REST-Endpunkte
│   └── mqtt-additions.md           # Neue MQTT-Topics
└── tasks.md                        # Phase 2 output (/speckit-tasks)
```

### Source Code (Erweiterungen im Repository-Root)

Neue Dateien (ergänzend zu Feature 001):

```text
src/
├── ShellyClient.h/.cpp             # HTTP-Client für Shelly Gen3 RPC API
├── SceneManager.h/.cpp             # Sequenz-Orchestrierung (start/stop)
├── ShellyMqtt.h/.cpp               # MQTT Handler für neue cmnd/stat Topics
└── AdminHandler.h/.cpp             # Admin-UI-Route + Passwort-Authentifizierung

data/
├── index.html                      # Erweitert: Szenen-Buttons + Licht/Leinwand-Karten
├── admin.html                      # Neu: Admin-Konfigurationsseite
├── style.css                       # Erweitert: Styles für neue Karten
└── app.js                          # Erweitert: Szenen- und Shelly-API-Calls
```

Bestehende Dateien, die erweitert werden:

```text
src/
├── ConfigManager.h/.cpp            # Erweiterung: ShellyConfig NVS-Namespace
├── RestHandler.h/.cpp              # Erweiterung: Neue Routen registrieren
├── MqttManager.h/.cpp              # Erweiterung: Subscribe + Publish für neue Topics
└── main.cpp                        # Erweiterung: ShellyClient + SceneManager init
```

**Structure Decision**: Additive Erweiterung — neue Module (`ShellyClient`, `SceneManager`,
`ShellyMqtt`, `AdminHandler`) folgen dem bestehenden Modul-Muster aus Feature 001.
Kein Umbau bestehender Module nötig. Web-UI-Dateien werden inplace erweitert.

## Phase 0: Research (abgeschlossen)

**Ergebnisse in**: [research.md](research.md)

Gelöste Unklarheiten:
- Shelly Gen3 HTTP RPC API: identisch mit Gen2 (gleiche Methoden, gleiche JSON-Schemas)
- Shelly 1 Mini Gen3: `Switch.Set?id=0&on=true/false`, `Switch.GetStatus?id=0`
- Shelly 2PM Gen3 (Cover-Modus): `Cover.Open/Close/Stop/GoToPosition?id=0`, `Cover.GetStatus?id=0`
- HTTP-Timeout: 3 s pro Request (Shelly antwortet lokal in < 200 ms)
- Admin-Auth: HTTP Basic Auth, SHA-256-Passwort-Hash in NVS
- Keine Extra-Libraries: `HTTPClient` ist Arduino-ESP32-built-in

## Phase 1: Design (abgeschlossen)

**Ergebnisse in**: [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md)

### Entscheidungen

**ShellyClient**:
- Synchrone `HTTPClient`-Calls (blocking, max 3 s timeout)
- Gibt `bool success` + optional geparsten Status zurück
- Kein Retry — bei Fehler → `reachable=false` melden, Sequenz fortsetzen

**SceneManager**:
- Führt Szenensequenz synchron aus (sequenziell, jeder Schritt wartet auf HTTP-Response)
- Gibt `SceneResult` (JSON-serialisierbar) zurück
- Kein globaler Lock — gleichzeitige Szenenaufrufe werden nicht koordiniert
  (akzeptables Verhalten bei Low-Concurrency Feuerwehr-Kontext)

**AdminHandler**:
- Servier `data/admin.html` aus LittleFS (wie `index.html`)
- HTTP Basic Auth mit SHA-256-Vergleich im Handler
- POST `/admin/save` parsed URL-encoded Formular, validiert IPs, speichert in NVS
- Neue IPs werden sofort in `ShellyClient`-Instanzen übernommen

**Web-UI-Erweiterungen**:
- `index.html`: Neue Sektionen "Szenen" (2 Buttons) + "Licht" (2 Buttons) + "Leinwand" (3 Buttons)
- `app.js`: `startScene()`, `stopScene()`, `setLight()`, `setScreen()` — gleiche `apiFetch()`-Pattern
- Status-Polling erweitert: alle 5 s auch `/api/light/status` und `/api/screen/status` abfragen
