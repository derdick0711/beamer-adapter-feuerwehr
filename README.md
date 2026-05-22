# Beamer Adapter Feuerwehr

RS232-WiFi-Adapter für den Acer H6512BD Beamer. Ermöglicht die Steuerung des Beamers
über REST API, MQTT und eine integrierte Web-Oberfläche — gebaut auf dem ESP32-S3 mit
PlatformIO/Arduino-ESP32.

## Funktionen

- **Beamer ein-/ausschalten** via RS232
- **Eingang auswählen** (HDMI, VGA, Component, S-Video, Composite)
- **Bild schwarz schalten** (Blank/Mute)
- **Status abfragen** (Betriebszustand, aktiver Eingang)
- **REST API** für direkte HTTP-Steuerung
- **MQTT** für Integration in Automationssysteme (Home Assistant, Node-RED)
- **Web-Oberfläche** unter der IP-Adresse des Adapters
- **Captive-Portal-Konfiguration** beim ersten Start (oder nach Reset)
- **Statische IP** konfigurierbar

---

## Hardware

### Komponenten

| Bauteil | Modell |
|---------|--------|
| Mikrocontroller | diymore ESP32-S3 DevKitC-1 N16R8 (16 MB Flash, 8 MB PSRAM) |
| RS232-Level-Konverter | MAX3232-Board (3,3 V ↔ RS232 ±12 V) |
| Beamer | Acer H6512BD (DLP, RS232 DB-9) |
| Verbindung ESP32 ↔ MAX3232 | Jumperkabel |
| Verbindung MAX3232 ↔ Beamer | RS232-Kabel DB-9 (Nullmodem oder 1:1 je nach Pinbelegung) |

### Verdrahtung ESP32-S3 ↔ MAX3232

```
ESP32-S3 DevKitC-1          MAX3232 Board
─────────────────           ─────────────
GPIO17 (TX, UART1)  ──────► T1IN  (TTL-Eingang)
GPIO16 (RX, UART1)  ◄─────── R1OUT (TTL-Ausgang)
3.3V                ──────── VCC
GND                 ──────── GND

MAX3232 Board          Beamer DB-9 (RS232)
─────────────          ───────────────────
T1OUT (RS232-TX) ─────► Pin 2 (RXD)
R1IN  (RS232-RX) ◄───── Pin 3 (TXD)
GND              ──────  Pin 5 (GND)
```

> **Hinweis**: Die genaue Pin-Belegung des MAX3232-Boards kann je nach Hersteller
> variieren. Beschriftung auf dem Board beachten. Für den RS232-Anschluss am Beamer
> immer die DB-9-Buchse des Acer H6512BD verwenden (nicht den HDMI- oder VGA-Port).

### Reset-Button

Der **BOOT-Button** (GPIO0) auf dem ESP32-S3 DevKit dient als WLAN-Reset:
- **5 Sekunden gedrückt halten** → WLAN-Konfiguration wird gelöscht, Adapter startet
  im Access-Point-Modus neu.

---

## Software

### Voraussetzungen

- [VS Code](https://code.visualstudio.com/) mit [PlatformIO-Extension](https://platformio.org/install/ide?install=vscode)
- Git

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
│  Strom                              │
│  ┌──────────────┐ ┌──────────────┐  │
│  │  Einschalten │ │ Ausschalten  │  │  ← blau / rot
│  └──────────────┘ └──────────────┘  │
├─────────────────────────────────────┤
│  Eingang                            │
│  ┌──────┐ ┌─────┐ ┌───────────┐    │
│  │ HDMI │ │ VGA │ │ Component │    │
│  └──────┘ └─────┘ └───────────┘    │
│  ┌─────────┐ ┌───────────┐         │
│  │ S-Video │ │ Composite │         │  ← aktiver Eingang hervorgehoben
│  └─────────┘ └───────────┘         │
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

### REST API

| Methode | Endpoint | Beschreibung |
|---------|----------|--------------|
| GET | `/api/status` | Aktueller Beamer-Status |
| POST | `/api/power` | Ein/Aus: `{"state":"on"}` |
| POST | `/api/input` | Eingang: `{"input":"hdmi"}` |
| POST | `/api/blank` | Schwarzbild: `{"enabled":true}` |

Eingänge: `hdmi`, `vga`, `component`, `svideo`, `composite`

Vollständige API-Dokumentation: [specs/001-rs232-wifi-adapter/contracts/rest-api.md](specs/001-rs232-wifi-adapter/contracts/rest-api.md)

### MQTT

| Topic | Richtung | Beispiel-Payload |
|-------|----------|-----------------|
| `beamer/cmnd/power` | → Adapter | `on` / `off` |
| `beamer/cmnd/input` | → Adapter | `hdmi` |
| `beamer/cmnd/blank` | → Adapter | `true` / `false` |
| `beamer/stat` | ← Adapter | JSON-Statusobjekt |

Topic-Präfix `beamer` ist in der Konfiguration änderbar.

Vollständige MQTT-Dokumentation: [specs/001-rs232-wifi-adapter/contracts/mqtt-contract.md](specs/001-rs232-wifi-adapter/contracts/mqtt-contract.md)

---

## RS232-Protokoll (Acer H6512BD)

| Funktion | RS232-Befehl |
|----------|-------------|
| Power ON | `*0 IR 001\r` |
| Power OFF | `*0 IR 002\r` |
| Eingang HDMI | `C36\r` |
| Eingang VGA | `C05\r` |
| Eingang Component | `C33\r` |
| Eingang S-Video | `C34\r` |
| Eingang Composite | `C35\r` |
| Schwarzbild | `*0 IR 055\r` |

Serielle Parameter: 9600 Baud, 8N1, kein Handshake.

---

## Projektdokumentation

- [Spezifikation](specs/001-rs232-wifi-adapter/spec.md)
- [Implementierungsplan](specs/001-rs232-wifi-adapter/plan.md)
- [Datenmodell](specs/001-rs232-wifi-adapter/data-model.md)
- [REST-API-Vertrag](specs/001-rs232-wifi-adapter/contracts/rest-api.md)
- [MQTT-Vertrag](specs/001-rs232-wifi-adapter/contracts/mqtt-contract.md)
- [Quickstart](specs/001-rs232-wifi-adapter/quickstart.md)
