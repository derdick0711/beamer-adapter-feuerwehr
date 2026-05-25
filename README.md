# Beamer Adapter Feuerwehr

RS232-WiFi-Adapter für den Acer H6512BD Beamer. Ermöglicht die Steuerung des Beamers
über REST API, MQTT und eine integrierte Web-Oberfläche — gebaut auf dem WeMos D1 Mini
(ESP8266) mit PlatformIO/Arduino.

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
- **Web-Oberfläche** mit IP-Anzeige und MQTT-Statusanzeige
- **Admin-Oberfläche** zur Konfiguration (Shelly-IPs, WLAN, statische IP, MQTT, Zugangsdaten)
- **Captive-Portal-Konfiguration** beim ersten Start oder nach WLAN-Reset
- **Statische IP** konfigurierbar
- **OTA-Updates** automatisch via GitHub Releases (stündlich)

---

## Hardware

### Komponenten

| Bauteil | Modell |
|---------|--------|
| Mikrocontroller | WeMos D1 Mini (ESP8266, 4 MB Flash) |
| RS232-Level-Konverter | MAX3232-Board (3,3 V ↔ RS232 ±12 V) |
| Beamer | Acer H6512BD (DLP, RS232 DB-9) |
| Verbindung ESP8266 ↔ MAX3232 | Jumperkabel |
| Verbindung MAX3232 ↔ Beamer | RS232-Kabel DB-9 |

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
WeMos D1 Mini          MAX3232 Board (TTL-Seite)
─────────────          ─────────────────────────
D1 / GPIO5 (TX)  ────► RX  (T1IN / T2IN)
D2 / GPIO4 (RX)  ◄──── TX  (R1OUT / R2OUT)
3.3V             ────── VCC
GND              ────── GND

MAX3232 Board (RS232-Seite)    Beamer DB-9
───────────────────────────    ──────────
T1OUT (RS232-TX)         ────► Pin 2 (RXD)
R1IN  (RS232-RX)         ◄──── Pin 3 (TXD)
GND                      ────── Pin 5 (GND)
```

> **Hinweis**: Auf der TTL-Seite (D1 Mini ↔ MAX3232) müssen TX und RX gekreuzt werden:
> D1 Mini TX → MAX3232 RX und D1 Mini RX ← MAX3232 TX. Die Beschriftung auf dem
> MAX3232-Board kann je nach Hersteller variieren.

---

## Hardware: Shelly-Geräte

| Gerät | Modell | Funktion |
|-------|--------|----------|
| Deckenlicht | Shelly 1 Mini Gen3 | Ein/Aus per HTTP RPC (`Switch.Set`) |
| Leinwand | Shelly 2PM Gen3 | Auf/Ab/Stop/Position per HTTP RPC (`Cover.*`) |

> **Hinweis**: Der Shelly 2PM Gen3 muss vor der ersten Nutzung in der Shelly-App
> unter Einstellungen → Betriebsmodus auf **Jalousiesteuerung / Roller** umgestellt
> und kalibriert werden. Anschließend IPs über `/admin` konfigurieren.

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
| ESPAsyncWebServer | ^3.6.0 | Asynchroner Web- und REST-Server |
| ESPAsyncTCP | ^2.0.0 | TCP-Basis für ESPAsyncWebServer |
| knolleary/PubSubClient | ^2.8 | MQTT-Client |
| bblanchon/ArduinoJson | ^7.0.0 | JSON-Serialisierung/-Deserialisierung |

Eingebaut (keine `lib_deps` nötig):
- **ESP8266HTTPClient** — HTTP-Aufrufe an Shelly-Geräte und GitHub API
- **ESP8266httpUpdate** — OTA-Updates via HTTPS
- **BearSSL** — SHA-256-Hashing für Admin-Passwort, TLS für OTA
- **EspSoftwareSerial** — RS232-Kommunikation auf D1/D2
- **LittleFS** — Konfigurationsspeicherung und Web-UI-Dateien

### Build & Flash

```bash
# Repository klonen
git clone <repo-url>
cd beamer-adapter-feuerwehr

# Firmware bauen und flashen
pio run -t upload -e d1_mini

# Filesystem (Web-UI) flashen
pio run -t uploadfs -e d1_mini

# Seriellen Monitor öffnen
pio device monitor -e d1_mini
```

### Initiale WLAN-Konfiguration

1. Nach dem ersten Flash erscheint der WLAN-Hotspot **`BeamerAdapter-Setup`**.
2. Mit Smartphone/PC verbinden (kein Passwort erforderlich).
3. Konfigurationsseite öffnet sich automatisch (Captive Portal). Falls nicht: `192.168.4.1` im Browser öffnen.
4. WLAN-SSID auswählen und Passwort eingeben.
5. Optional: Statische IP-Adresse, Gateway und Subnetz eintragen.
6. Optional: MQTT-Broker-Adresse und Port konfigurieren.
7. **Speichern** → Adapter verbindet sich mit dem WLAN.

---

## Nutzung

### Web-Oberfläche

Browser öffnen: `http://<adapter-ip>/`

Die Seite ist für Smartphone und Desktop optimiert.

#### Status-Badges (Header)

| Badge | Bedeutung |
|-------|-----------|
| **EIN** / **AUS** (grün/grau) | Betriebszustand des Beamers |
| **HDMI** / **VGA** | Aktiver Eingang |
| **Schwarz** | Bild ist schwarz geschaltet |
| **Offline** (gelb) | Beamer antwortet nicht über RS232 |
| **192.168.x.x** | Aktuelle IP-Adresse des Adapters |
| **MQTT: ON** | MQTT-Broker ist verbunden |

Status wird alle 5 Sekunden automatisch aktualisiert.

---

### Admin-Oberfläche

Browser öffnen: `http://<adapter-ip>/admin`

Passwortgeschützt (Standard-Benutzername: **`admin`**, Standard-Passwort: **`feuerwehr`**).

#### Konfigurierbare Bereiche

| Bereich | Felder |
|---------|--------|
| **Shelly** | IP des Shelly 1 Mini (Licht), IP des Shelly 2PM (Leinwand) — mit Status-Dot |
| **WLAN** | SSID, Passwort, statische IP (Adresse / Gateway / Subnetz) |
| **MQTT** | Aktivieren/Deaktivieren, Broker-IP, Port, Topic-Präfix |
| **Zugangsdaten** | Benutzername und Passwort ändern |

> Änderungen an WLAN und MQTT werden erst nach einem Neustart aktiv. Die Seite zeigt
> nach dem Speichern ein entsprechendes Banner mit Neustart-Button.

**Partielles Speichern**: Leere Felder überschreiben keine bestehenden Werte.

#### WLAN zurücksetzen

Im Admin-Bereich gibt es den Button **„WLAN zurücksetzen"**. Dieser löscht die gespeicherten
WLAN-Zugangsdaten und startet den Adapter im AP-Modus neu (`BeamerAdapter-Setup`).

**Physischer Fallback**: BOOT-Taste (D3/GPIO0) beim Einschalten gedrückt halten
und erst nach ~2 Sekunden loslassen → WLAN-Reset und AP-Modus.

Das Admin-Passwort wird als SHA-256-Hash gespeichert (nicht im Klartext).

---

### REST API

Basis-URL: `http://<adapter-ip>`

#### Beamer

| Methode | Endpoint | Body | Beschreibung |
|---------|----------|------|--------------|
| GET | `/api/status` | — | Aktueller Beamer-Status inkl. IP und MQTT-Zustand |
| POST | `/api/power` | `{"state":"on"\|"off"}` | Ein-/Ausschalten |
| POST | `/api/input` | `{"input":"hdmi"\|"vga"}` | Eingang wählen |
| POST | `/api/blank` | `{"enabled":true\|false}` | Schwarzbild ein/aus |

**Beispiel-Antwort** `GET /api/status`:
```json
{
  "power": "on",
  "input": "hdmi",
  "blank": false,
  "reachable": true,
  "lastUpdated": 12345678,
  "ip": "192.168.1.50",
  "mqttConnected": true
}
```

#### Szenen

| Methode | Endpoint | Beschreibung |
|---------|----------|--------------|
| POST | `/api/scene/start` | Präsentation starten (Leinwand↓ → Licht aus → Beamer ein → HDMI) |
| POST | `/api/scene/stop` | Präsentation beenden (Leinwand↑ → Licht an → Beamer aus) |

**Beispiel-Antwort**:
```json
{
  "scene": "start",
  "success": true,
  "duration_ms": 2340,
  "steps": [
    {"device": "screen", "action": "close",     "success": true, "error": ""},
    {"device": "light",  "action": "off",        "success": true, "error": ""},
    {"device": "beamer", "action": "power_on",   "success": true, "error": ""},
    {"device": "beamer", "action": "input_hdmi", "success": true, "error": ""}
  ]
}
```

#### Deckenlicht (Shelly 1 Mini Gen3)

| Methode | Endpoint | Body | Beschreibung |
|---------|----------|------|--------------|
| GET | `/api/light/status` | — | Schaltzustand + Erreichbarkeit |
| POST | `/api/light` | `{"state":"on"\|"off"}` | Licht ein-/ausschalten |

#### Leinwand (Shelly 2PM Gen3)

| Methode | Endpoint | Body | Beschreibung |
|---------|----------|------|--------------|
| GET | `/api/screen/status` | — | Fahrzustand + Position (0–100 %) |
| POST | `/api/screen` | `{"action":"open"}` | Hochfahren |
| POST | `/api/screen` | `{"action":"close"}` | Herunterfahren |
| POST | `/api/screen` | `{"action":"stop"}` | Anhalten |
| POST | `/api/screen` | `{"action":"position","pos":75}` | Zielposition 0–100 % |

#### Admin-Endpunkte (Passwortgeschützt)

| Methode | Endpoint | Beschreibung |
|---------|----------|--------------|
| GET | `/admin` | Konfigurationsseite |
| GET | `/admin/config` | Aktuelle Konfiguration als JSON |
| POST | `/admin/save` | Konfiguration speichern |
| POST | `/admin/restart` | Gerät neu starten |
| POST | `/admin/reset-wifi` | WLAN-Credentials löschen und AP-Modus starten |

---

### MQTT

Topic-Präfix ist konfigurierbar (Standard: `beamer`).

#### Befehle (→ Adapter)

| Topic | Payload | Beschreibung |
|-------|---------|--------------|
| `beamer/cmnd/power` | `on` / `off` | Beamer ein-/ausschalten |
| `beamer/cmnd/input` | `hdmi` / `vga` | Eingang wählen |
| `beamer/cmnd/blank` | `true` / `false` | Schwarzbild ein/aus |
| `beamer/cmnd/light` | `on` / `off` | Licht steuern |
| `beamer/cmnd/screen` | `open` / `close` / `stop` / `pos:75` | Leinwand steuern |
| `beamer/cmnd/scene/start` | beliebig | Präsentation starten |
| `beamer/cmnd/scene/stop` | beliebig | Präsentation beenden |

#### Status (← Adapter, retained)

| Topic | Payload | Beschreibung |
|-------|---------|--------------|
| `beamer/stat` | JSON | Beamer-Status (power, input, blank, reachable, uptime) |
| `beamer/stat/light` | JSON | Licht-Status (output, reachable) |
| `beamer/stat/screen` | JSON | Leinwand-Status (state, current_pos, reachable) |
| `beamer/stat/scene` | JSON | Szenen-Ergebnis (nicht retained) |
| `beamer/ip` | `192.168.x.x` | Aktuelle IP-Adresse (retained, wird bei jedem Connect aktualisiert) |

**LWT** (Last Will and Testament): Bei Verbindungsabbruch wird automatisch
`{"reachable":false,"reason":"lwt"}` auf `beamer/stat` veröffentlicht.

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

## OTA-Updates

Der Adapter prüft stündlich auf neue Releases im GitHub-Repository. Bei einem neueren
`firmware.bin`-Asset wird automatisch geflasht und neu gestartet. Die aktuelle Firmware-Version
ist im seriellen Monitor beim Boot sichtbar (`[OTA] version=...`).

---

## Quellcode-Übersicht

```
src/
├── main.cpp              Initialisierung, Setup, Loop
├── ConfigManager.h/.cpp  Konfiguration (WLAN, MQTT, Shelly-IPs, Admin-PW) in LittleFS
├── WifiProvisioner.h/.cpp  WLAN-Verbindung, Captive Portal, Boot-Reset
├── BeamerRS232.h/.cpp    RS232-Kommunikation (SoftwareSerial D1/D2) mit Acer H6512BD
├── BeamerStatus.h        Globaler Beamer-Zustand (power, input, blank, reachable)
├── RestHandler.h/.cpp    ESPAsyncWebServer — REST-Routen
├── AdminHandler.h/.cpp   Admin-Webseite mit HTTP Basic Auth + SHA-256
├── MqttManager.h/.cpp    PubSubClient — MQTT pub/sub, LWT, retained IP
├── ShellyClient.h/.cpp   HTTP-RPC-Client für Shelly Gen3 (Switch & Cover)
├── SceneManager.h/.cpp   Szenen-Automatisierung (start/stop)
└── OtaUpdater.h/.cpp     Stündliche OTA-Prüfung via GitHub Releases API

data/
├── index.html            Web-Oberfläche (IP-Badge, MQTT-Badge)
├── admin.html            Admin-Konfigurationsseite
├── app.js                Frontend-JavaScript (Polling, API-Aufrufe)
└── style.css             CSS (Mobile-first)
```

---

## Projektdokumentation

### Feature 001 — RS232-WiFi Beamer Adapter
- [Spezifikation](specs/001-rs232-wifi-adapter/spec.md)
- [Implementierungsplan](specs/001-rs232-wifi-adapter/plan.md)
- [REST-API-Vertrag](specs/001-rs232-wifi-adapter/contracts/rest-api.md)
- [MQTT-Vertrag](specs/001-rs232-wifi-adapter/contracts/mqtt-contract.md)

### Feature 002 — Shelly-Integration & Präsentationsautomatisierung
- [Spezifikation](specs/002-shelly-automation/spec.md)
- [Implementierungsplan](specs/002-shelly-automation/plan.md)
- [REST-API-Erweiterungen](specs/002-shelly-automation/contracts/rest-api-additions.md)
- [MQTT-Erweiterungen](specs/002-shelly-automation/contracts/mqtt-additions.md)

### Feature 003 — Eingangswahl auf HDMI und VGA reduzieren
- [Spezifikation](specs/003-reduce-inputs/spec.md)

### Feature 004 — Admin-UI Status & Konfiguration
- [Spezifikation](specs/004-admin-ui-status/spec.md)
- [Implementierungsplan](specs/004-admin-ui-status/plan.md)
- [REST-API-Änderungen](specs/004-admin-ui-status/contracts/rest-api-changes.md)
