# Quickstart: RS232-WiFi Beamer Adapter

## Prerequisites

- VS Code mit PlatformIO-Extension installiert
- ESP32-S3 DevKitC-1 N16R8 per USB-C angeschlossen
- MAX3232 RS232-Board verdrahtet (siehe Hardware-Anleitung in README.md)
- Acer H6512BD Beamer per RS232-Kabel (DB-9) am MAX3232 angeschlossen

---

## 1. Projekt öffnen & flashen

```bash
# Repository klonen / Ordner in VS Code öffnen
code .

# PlatformIO: Build + Upload
pio run -t upload

# Seriellen Monitor öffnen (9600 baud für Debug-Ausgaben)
pio device monitor
```

---

## 2. Initiale WLAN-Konfiguration

1. Nach dem ersten Flash öffnet der Adapter den Access Point **`BeamerAdapter-Setup`**.
2. Mit einem Smartphone oder PC mit dem AP verbinden (kein Passwort).
3. Browser öffnet automatisch die Konfigurationsseite (Captive Portal).
   Falls nicht: `192.168.4.1` im Browser eingeben.
4. **WLAN auswählen** → SSID aus der Liste wählen → Passwort eingeben.
5. Optional: **Statische IP** aktivieren und IP-Adresse, Gateway, Subnetzmaske eingeben.
6. Optional: **MQTT-Broker** konfigurieren (IP/Hostname, Port, Topic-Präfix).
7. **Speichern** drücken → Adapter startet neu und verbindet sich mit dem WLAN.

---

## 3. Beamer steuern

### REST API (curl-Beispiele)

```bash
# Status abfragen
curl http://<adapter-ip>/api/status

# Einschalten
curl -X POST http://<adapter-ip>/api/power \
     -H "Content-Type: application/json" \
     -d '{"state":"on"}'

# Eingang auf HDMI umschalten
curl -X POST http://<adapter-ip>/api/input \
     -H "Content-Type: application/json" \
     -d '{"input":"hdmi"}'

# Bild schwarz schalten
curl -X POST http://<adapter-ip>/api/blank \
     -H "Content-Type: application/json" \
     -d '{"enabled":true}'
```

### Web-Oberfläche

Browser öffnen: `http://<adapter-ip>/`

### MQTT (mosquitto-Beispiele)

```bash
# Beamer einschalten
mosquitto_pub -h <broker-ip> -t beamer/cmnd/power -m on

# HDMI auswählen
mosquitto_pub -h <broker-ip> -t beamer/cmnd/input -m hdmi

# Status empfangen
mosquitto_sub -h <broker-ip> -t beamer/stat
```

---

## 4. WLAN zurücksetzen

Den **BOOT-Button** (GPIO0) auf dem ESP32-S3 DevKit **5 Sekunden** gedrückt halten.
Der Adapter löscht die gespeicherten WLAN-Daten und startet im Access-Point-Modus neu.

---

## 5. Validierung (Hardware-in-the-Loop)

```bash
# 1. Adapter geflasht und im WLAN → Status abrufen
curl http://<adapter-ip>/api/status
# Erwartung: {"power":"unknown","reachable":true,...}

# 2. Beamer einschalten
curl -X POST http://<adapter-ip>/api/power -d '{"state":"on"}' -H "Content-Type: application/json"
# Erwartung: {"power":"on","reachable":true}

# 3. MQTT testen
mosquitto_pub -h <broker-ip> -t beamer/cmnd/blank -m true
mosquitto_sub -h <broker-ip> -t beamer/stat
# Erwartung: {"blank":true,"reachable":true,...}
```
