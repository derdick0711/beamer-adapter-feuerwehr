# Beamer Adapter Feuerwehr

RS232-WiFi-Adapter für den Acer H6512BD Beamer. Ermöglicht die Steuerung des Beamers
über REST API, MQTT und eine integrierte Web-Oberfläche — gebaut auf dem ESP32-S3 mit
PlatformIO/Arduino-ESP32.

## Funktionen

- **Beamer ein-/ausschalten** via RS232
- **Eingang auswählen** (HDMI, VGA)
- **Bild schwarz schalten** (Blank/Mute)
- **Status abfragen** (Betriebszustand, aktiver Eingang)
- **Präsentation starten** (Ein-Knopf: Leinwand runter → Licht aus → Beamer ein → HDMI)
- **Präsentation beenden** (Ein-Knopf: Leinwand hoch → Licht an → Beamer aus)
- **Deckenlicht steuern** (Shelly 1 Mini Gen3 — ein/aus)
- **Leinwand steuern** (Shelly 2PM Gen3 — auf/ab/stop/Position 0–100 %)
- **REST API** für direkte HTTP-Steuerung aller Funktionen
- **MQTT** für Integration in Automationssysteme (Home Assistant, Node-RED)
- **Web-Oberfläche** unter der IP-Adresse des Adapters
- **Admin-Oberfläche** zur Konfiguration der Shelly-IPs (passwortgeschützt)
- **Captive-Portal-Konfiguration** beim ersten Start (oder nach Reset)
- **Statische IP** konfigurierbar

---

## Hardware

### Komponenten

| Bauteil | Modell |
|---------|--------|
| Mikrocontroller | ARCELI WeMos WiFi ESP8266 D1 Mini (4 MB Flash) |
| RS232-Level-Konverter | MAX3232-Board (3,3 V ↔ RS232 ±12 V) |
| Beamer | Acer H6512BD (DLP, RS232 DB-9) |
| Verbindung ESP8266 ↔ MAX3232 | Jumperkabel |
| Verbindung MAX3232 ↔ Beamer | RS232-Kabel DB-9 (Nullmodem oder 1:1 je nach Pinbelegung) |

### WeMos D1 Mini Pinbelegung

```
WeMos D1 Mini    GPIO    Funktion
─────────────    ────    ────────
D1               GPIO5   RS232 TX  →  MAX3232 T1IN
D2               GPIO4   RS232 RX  ←  MAX3232 R1OUT
D3               GPIO0   WLAN-Reset-Button (BOOT)
3.3V             —       MAX3232 VCC
GND              —       MAX3232 GND / Beamer Pin 5
```

### Verdrahtung WeMos D1 Mini ↔ MAX3232

```
WeMos D1 Mini          MAX3232 Board
─────────────          ─────────────
D1 / GPIO5 (TX)  ────► T1IN  (TTL-Eingang)
D2 / GPIO4 (RX)  ◄──── R1OUT (TTL-Ausgang)
3.3V             ────── VCC
GND              ────── GND

MAX3232 Board          Beamer DB-9 (RS232)
─────────────          ───────────────────
T1OUT (RS232-TX) ────► Pin 2 (RXD)
R1IN  (RS232-RX) ◄──── Pin 3 (TXD)
GND              ──────  Pin 5 (GND)
```

> **Hinweis**: Die genaue Pin-Belegung des MAX3232-Boards kann je nach Hersteller
> variieren. Beschriftung auf dem Board beachten. Für den RS232-Anschluss am Beamer
> immer die DB-9-Buchse des Acer H6512BD verwenden (nicht den HDMI- oder VGA-Port).

### Reset-Button

Der **D3-Button** (GPIO0 / FLASH-Button) auf dem WeMos D1 Mini dient als WLAN-Reset:
- **5 Sekunden gedrückt halten** → WLAN-Konfiguration wird gelöscht, Adapter startet
  im Access-Point-Modus neu.

---

## Hardware: Shelly-Geräte

| Gerät | Modell | Funktion |
|-------|--------|----------|
| Deckenlicht | Shelly 1 Mini Gen3 | Ein/Aus per HTTP RPC (`Switch.Set`) |
| Leinwand | Shelly 2PM Gen3 | Auf/Ab/Stop/Position per HTTP RPC (`Cover.*`) — muss im Roller-Modus konfiguriert sein |

> **Hinweis**: Der Shelly 2PM Gen3 muss vor der ersten Nutzung in der Shelly-App
> unter Einstellungen → Betriebsmodus auf **Jalousiesteuerung / Roller** umgestellt
> und kalibriert werden. Anschließend IPs über `/admin` im Browser konfigurieren.

---

## Software

### Voraussetzungen

- [VS Code](https://code.visualstudio.com/) mit [PlatformIO-Extension](https://platformio.org/install/ide?install=vscode)
- Git

### Abhängigkeiten

Alle Bibliotheken werden von PlatformIO automatisch installiert (`platformio.ini`):

| Bibliothek | Version | Verwendung |
|-----------|---------|-----------|
| tzapu/WiFiManager | ^2.0.17 | Captive-Portal-Konfiguration |
| me-no-dev/ESPAsyncWebServer | ^1.2.3 | Asynchroner Web- und REST-Server |
| me-no-dev/ESPAsyncTCP | ^1.2.3 | TCP-Basis für ESPAsyncWebServer (ESP8266) |
| knolleary/PubSubClient | ^2.8 | MQTT-Client |
| bblanchon/ArduinoJson | ^7.0.0 | JSON-Serialisierung/-Deserialisierung |

Eingebaut (keine `lib_deps` nötig):
- **ESP8266HTTPClient** (Arduino-ESP8266) — HTTP-Aufrufe an Shelly-Geräte
- **BearSSL** (ESP8266 Arduino Core) — SHA-256-Hashing für Admin-Passwort
- **SoftwareSerial** (ESP8266 Arduino Core) — RS232-Kommunikation auf D1/D2
- **LittleFS** — persistente Konfigurationsspeicherung (JSON-Datei `/config.json`)

### Build & Flash

```bash
# Repository klonen
git clone <repo-url>
cd beamer-adapter-feuerwehr

# In VS Code öffnen
code .

# PlatformIO: Firmware bauen und flashen
pio run -t upload

# Filesystem (Web-UI) flashen
pio run -t uploadfs

# Seriellen Monitor öffnen (für Debug-Ausgaben)
pio device monitor
```

> **Debug-Build**: `pio run -e esp32-s3-devkitc-1-debug -t upload` aktiviert
> ausführliche Logging-Ausgaben über den seriellen Monitor.

### Initiale WLAN-Konfiguration

1. Nach dem ersten Flash erscheint der WLAN-Hotspot **`BeamerAdapter-Setup`**.
2. Mit Smartphone/PC verbinden (kein Passwort erforderlich).
3. Konfigurationsseite öffnet sich automatisch (Captive Portal).
   Falls nicht: Browser auf `192.168.4.1` öffnen.
4. WLAN-SSID auswählen und Passwort eingeben.
5. Optional: Statische IP-Adresse konfigurieren.
6. Optional: MQTT-Broker-Adresse konfigurieren.
7. **Speichern** → Adapter verbindet sich mit dem WLAN.

---

## Nutzung

### Web-Oberfläche

Browser öffnen: `http://<adapter-ip>/`

Die Seite ist für Smartphone und Desktop optimiert und benötigt keine App-Installation.

#### Aufbau der Seite

```
┌─────────────────────────────────────┐
│  Beamer Adapter                     │  ← blauer Header
│  [EIN] [HDMI] [Schwarz] [Offline?]  │  ← Status-Badges
├─────────────────────────────────────┤
│  Szenen                             │
│  ┌────────────────┐ ┌─────────────┐ │
│  │ Präsentation   │ │Präsentation │ │  ← blau / rot
│  │    starten     │ │  beenden    │ │
│  └────────────────┘ └─────────────┘ │
├─────────────────────────────────────┤
│  Licht ●                            │  ← grüner Punkt = an
│  ┌──────────┐ ┌──────────┐         │
│  │  Licht   │ │  Licht   │         │
│  │   ein    │ │   aus    │         │
│  └──────────┘ └──────────┘         │
├─────────────────────────────────────┤
│  Leinwand  75% – closing            │  ← Position + Fahrzustand
│  ┌────────┐ ┌────────┐ ┌────────┐  │
│  │ Runter │ │  Stop  │ │  Hoch  │  │
│  └────────┘ └────────┘ └────────┘  │
├─────────────────────────────────────┤
│  Strom                              │
│  ┌──────────────┐ ┌──────────────┐  │
│  │  Einschalten │ │ Ausschalten  │  │  ← blau / rot
│  └──────────────┘ └──────────────┘  │
├─────────────────────────────────────┤
│  Eingang                            │
│  ┌──────────┐ ┌──────────┐         │
│  │   HDMI   │ │   VGA    │         │  ← aktiver Eingang hervorgehoben
│  └──────────┘ └──────────┘         │
├─────────────────────────────────────┤
│  Bild                               │
│  ┌─────────────────┐ ┌───────────┐  │
│  │ Schwarz schalten│ │Bild frei- │  │
│  └─────────────────┘ │  geben   │  │
│                      └───────────┘  │
└─────────────────────────────────────┘
```

#### Status-Badges (oben im Header)

| Badge | Bedeutung |
|-------|-----------|
| **EIN** / **AUS** (grün/grau) | Aktueller Betriebszustand des Beamers |
| **HDMI** / **VGA** / … | Aktuell aktiver Eingang |
| **Schwarz** (erscheint nur wenn aktiv) | Bild ist schwarz geschaltet |
| **Offline** (gelb, erscheint nur bei Fehler) | Beamer antwortet nicht über RS232 |

#### Verhalten

- **Status-Aktualisierung**: Die Seite fragt den Adapter alle 5 Sekunden automatisch ab — kein manuelles Neuladen nötig.
- **Aktiver Eingang**: Der zuletzt gewählte Eingangsbutton wird blau hervorgehoben.
- **Fehlermeldungen**: Bei RS232-Timeout erscheint unten eine rote Toast-Meldung „Beamer antwortet nicht". Die Seite bleibt nutzbar und zeigt den zuletzt bekannten Status.
- **Touch-optimiert**: Alle Buttons sind mindestens 48 px hoch — auf dem Smartphone per Daumen bedienbar.
- **Kein Login erforderlich**: Die Seite ist ohne Passwort erreichbar (Absicherung über Netzwerksegmentierung).

### Admin-Oberfläche

Browser öffnen: `http://<adapter-ip>/admin`

Passwortgeschützt (Standard: **`feuerwehr`**). Hier können konfiguriert werden:

- IP-Adresse des Shelly 1 Mini Gen3 (Deckenlicht)
- IP-Adresse des Shelly 2PM Gen3 (Leinwand)
- Admin-Passwort ändern

Das Passwort wird als SHA-256-Hash im NVS-Flash gespeichert (nicht im Klartext).

---

### REST API

Basis-URL: `http://<adapter-ip>`

#### Beamer

| Methode | Endpoint | Body / Parameter | Beschreibung |
|---------|----------|-----------------|--------------|
| GET | `/api/status` | — | Aktueller Beamer-Status |
| POST | `/api/power` | `{"state":"on"\|"off"}` | Ein-/Ausschalten |
| POST | `/api/input` | `{"input":"hdmi"\|"vga"}` | Eingang wählen |
| POST | `/api/blank` | `{"enabled":true\|false}` | Schwarzbild ein/aus |

**Gültige Eingangswerte**: `hdmi`, `vga`
(Andere Werte werden mit HTTP 400 abgelehnt.)

**Beispiel-Antwort** `GET /api/status`:
```json
{
  "power": "on",
  "input": "hdmi",
  "blank": false,
  "reachable": true,
  "lastUpdated": 12345678
}
```

#### Szenen-Automatisierung

| Methode | Endpoint | Beschreibung |
|---------|----------|--------------|
| POST | `/api/scene/start` | Präsentation starten (Leinwand↓ → Licht aus → Beamer ein → HDMI) |
| POST | `/api/scene/stop` | Präsentation beenden (Leinwand↑ → Licht an → Beamer aus) |

**Beispiel-Antwort** `POST /api/scene/start`:
```json
{
  "scene": "start",
  "success": true,
  "duration_ms": 2340,
  "steps": [
    {"device": "screen", "action": "close",      "success": true,  "error": ""},
    {"device": "light",  "action": "off",         "success": true,  "error": ""},
    {"device": "beamer", "action": "power_on",    "success": true,  "error": ""},
    {"device": "beamer", "action": "input_hdmi",  "success": true,  "error": ""}
  ]
}
```

`success: false` bedeutet Teilfehler — einzelne Schritte können fehlschlagen, die Szene läuft trotzdem durch. Details stehen in `steps[].error`.

#### Deckenlicht (Shelly 1 Mini Gen3)

| Methode | Endpoint | Body | Beschreibung |
|---------|----------|------|--------------|
| GET | `/api/light/status` | — | Schaltzustand + Erreichbarkeit |
| POST | `/api/light` | `{"state":"on"\|"off"}` | Licht ein-/ausschalten |

**Beispiel-Antwort** `GET /api/light/status`:
```json
{"output": true, "reachable": true}
```

#### Leinwand (Shelly 2PM Gen3)

| Methode | Endpoint | Body | Beschreibung |
|---------|----------|------|--------------|
| GET | `/api/screen/status` | — | Fahrzustand + Position (0–100 %) |
| POST | `/api/screen` | `{"action":"open"}` | Leinwand hochfahren |
| POST | `/api/screen` | `{"action":"close"}` | Leinwand herunterfahren |
| POST | `/api/screen` | `{"action":"stop"}` | Leinwand anhalten |
| POST | `/api/screen` | `{"action":"position","pos":75}` | Zielposition 0–100 % |

**Beispiel-Antwort** `GET /api/screen/status`:
```json
{"state": "open", "current_pos": 100, "reachable": true}
```

Mögliche `state`-Werte: `open`, `closed`, `opening`, `closing`, `stopped`, `unknown`.

#### Konfiguration

| Methode | Endpoint | Beschreibung |
|---------|----------|--------------|
| GET | `/api/config` | Aktuelle Netzwerk- und Shelly-Konfiguration (keine Passwörter) |

#### Admin-Oberfläche

| Methode | Endpoint | Beschreibung |
|---------|----------|--------------|
| GET | `/admin` | Konfigurationsseite (Passwortgeschützt — Standard: `feuerwehr`) |
| POST | `/admin/save` | Shelly-IPs und Passwort speichern |

---

### MQTT

Topic-Präfix `beamer` ist in der Konfiguration änderbar (Standard: `beamer`).

#### Beamer-Befehle

| Topic | Richtung | Beispiel-Payload |
|-------|----------|-----------------|
| `beamer/cmnd/power` | → Adapter | `on` / `off` |
| `beamer/cmnd/input` | → Adapter | `hdmi` / `vga` |
| `beamer/cmnd/blank` | → Adapter | `true` / `false` |
| `beamer/stat` | ← Adapter | JSON-Statusobjekt (retained) |

Gültige Payload-Werte für `cmnd/input`: `hdmi`, `vga`
(Alle anderen Werte werden stillschweigend ignoriert.)

**Beispiel** `beamer/stat`:
```json
{
  "power": "on",
  "input": "hdmi",
  "blank": false,
  "reachable": true,
  "uptime": 3600
}
```

#### Szenen & Shelly

| Topic | Richtung | Payload |
|-------|----------|---------|
| `beamer/cmnd/scene/start` | → Adapter | beliebig (z. B. `1`) |
| `beamer/cmnd/scene/stop` | → Adapter | beliebig (z. B. `1`) |
| `beamer/cmnd/light` | → Adapter | `on` / `off` |
| `beamer/cmnd/screen` | → Adapter | `open` / `close` / `stop` / `pos:75` |
| `beamer/stat/scene` | ← Adapter | SceneResult JSON (nicht retained) |
| `beamer/stat/light` | ← Adapter | `{"output":true,"reachable":true}` (retained) |
| `beamer/stat/screen` | ← Adapter | `{"state":"open","current_pos":100,"reachable":true}` (retained) |

---

## RS232-Protokoll (Acer H6512BD)

| Funktion | RS232-Befehl |
|----------|-------------|
| Power ON | `*0 IR 001\r` |
| Power OFF | `*0 IR 002\r` |
| Eingang HDMI | `C36\r` |
| Eingang VGA | `C05\r` |
| Schwarzbild | `*0 IR 055\r` |

Serielle Parameter: 9600 Baud, 8N1, kein Handshake.
ACK = `0x06`, NAK = `0x15`, Timeout = 500 ms.

---

## Quellcode-Übersicht

```
src/
├── main.cpp              Initialisierung, Setup, Loop
├── ConfigManager.h/.cpp  NVS-Konfiguration (WLAN, MQTT, Shelly-IPs, Admin-PW)
├── BeamerRS232.h/.cpp    RS232-Kommunikation (SoftwareSerial D1/D2) mit Acer H6512BD
├── BeamerStatus.h        Globaler Beamer-Zustand (power, input, blank, reachable)
├── RestHandler.h/.cpp    ESPAsyncWebServer — REST-Routen
├── MqttManager.h/.cpp    PubSubClient — MQTT pub/sub
├── ShellyClient.h/.cpp   HTTP-RPC-Client für Shelly Gen3 (Switch & Cover)
├── SceneManager.h/.cpp   Szenen-Automatisierung (start/stop)
└── AdminHandler.h/.cpp   Admin-Webseite mit HTTP Basic Auth + SHA-256

data/
├── index.html            Web-Oberfläche (Hauptseite)
├── admin.html            Admin-Konfigurationsseite
├── app.js                Frontend-JavaScript (Polling, API-Aufrufe)
└── style.css             CSS (Mobile-first, Google Material Design)
```

---

## Projektdokumentation

### Feature 001 — RS232-WiFi Beamer Adapter
- [Spezifikation](specs/001-rs232-wifi-adapter/spec.md)
- [Implementierungsplan](specs/001-rs232-wifi-adapter/plan.md)
- [Datenmodell](specs/001-rs232-wifi-adapter/data-model.md)
- [REST-API-Vertrag](specs/001-rs232-wifi-adapter/contracts/rest-api.md)
- [MQTT-Vertrag](specs/001-rs232-wifi-adapter/contracts/mqtt-contract.md)
- [Quickstart](specs/001-rs232-wifi-adapter/quickstart.md)

### Feature 002 — Shelly-Integration & Präsentationsautomatisierung
- [Spezifikation](specs/002-shelly-automation/spec.md)
- [Implementierungsplan](specs/002-shelly-automation/plan.md)
- [Datenmodell](specs/002-shelly-automation/data-model.md)
- [REST-API-Erweiterungen](specs/002-shelly-automation/contracts/rest-api-additions.md)
- [MQTT-Erweiterungen](specs/002-shelly-automation/contracts/mqtt-additions.md)
- [Quickstart](specs/002-shelly-automation/quickstart.md)

### Feature 003 — Eingangswahl auf HDMI und VGA reduzieren
- [Spezifikation](specs/003-reduce-inputs/spec.md)
