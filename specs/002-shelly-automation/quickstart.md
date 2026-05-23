# Quickstart: Shelly-Integration & Präsentationsautomatisierung

**Feature**: 002-shelly-automation
**Date**: 2026-05-23

---

## Voraussetzungen

- Feature 001 (RS232-WiFi Adapter) vollständig implementiert und im WLAN
- Shelly 1 Mini Gen3 im selben WLAN-Netz, IP bekannt
- Shelly 2PM Gen3 im selben WLAN-Netz, IP bekannt, **im Roller/Cover-Modus konfiguriert**
- Beamer-Adapter läuft unter bekannter IP (z.B. `192.168.1.50`)

---

## Shelly 2PM Gen3: Roller-Modus aktivieren

Vor der ersten Nutzung muss der Shelly 2PM Gen3 in den Roller/Cover-Modus versetzt werden:

1. Shelly-App öffnen → Gerät auswählen → Einstellungen → Betriebsmodus
2. **"Jalousiesteuerung / Roller"** auswählen
3. Kalibrierung durchführen (Auf/Ab-Laufzeit messen)
4. Position 0 % = Leinwand oben (eingerollt), 100 % = Leinwand unten (ausgerollt)

---

## Admin-Oberfläche: IPs konfigurieren

1. Browser öffnen: `http://<adapter-ip>/admin`
2. Passwort eingeben (Standard: `feuerwehr`)
3. IP-Adresse des Shelly 1 Mini Gen3 (Deckenlicht) eintragen
4. IP-Adresse des Shelly 2PM Gen3 (Leinwand) eintragen
5. **Speichern** → Adapter übernimmt neue IPs sofort (kein Neustart nötig)

---

## Szenario 1: Präsentation starten (REST)

```bash
curl -X POST http://<adapter-ip>/api/scene/start
```

Erwartete Antwort:
```json
{
  "scene": "start",
  "success": true,
  "duration_ms": 1842,
  "steps": [
    { "device": "screen",  "action": "close",    "success": true, "error": "" },
    { "device": "light",   "action": "off",      "success": true, "error": "" },
    { "device": "beamer",  "action": "power_on", "success": true, "error": "" },
    { "device": "beamer",  "action": "hdmi",     "success": true, "error": "" }
  ]
}
```

Ergebnis: Leinwand fährt herunter, Licht geht aus, Beamer schaltet ein, HDMI-Eingang aktiv.

---

## Szenario 2: Präsentation beenden (REST)

```bash
curl -X POST http://<adapter-ip>/api/scene/stop
```

Ergebnis: Leinwand fährt hoch, Licht geht an, Beamer schaltet aus.

---

## Szenario 3: Präsentation starten (MQTT)

```bash
mosquitto_pub -h <mqtt-broker> -t beamer/cmnd/scene/start -m "1"
```

Ergebnis verfolgen:
```bash
mosquitto_sub -h <mqtt-broker> -t beamer/stat/scene
```

---

## Szenario 4: Deckenlicht manuell schalten (REST)

```bash
# Licht ein
curl -X POST http://<adapter-ip>/api/light -H "Content-Type: application/json" -d '{"state":"on"}'

# Licht aus
curl -X POST http://<adapter-ip>/api/light -H "Content-Type: application/json" -d '{"state":"off"}'

# Status abfragen
curl http://<adapter-ip>/api/light/status
```

---

## Szenario 5: Leinwand manuell steuern (REST)

```bash
# Leinwand runter
curl -X POST http://<adapter-ip>/api/screen -H "Content-Type: application/json" -d '{"action":"close"}'

# Leinwand hoch
curl -X POST http://<adapter-ip>/api/screen -H "Content-Type: application/json" -d '{"action":"open"}'

# Leinwand stoppen
curl -X POST http://<adapter-ip>/api/screen -H "Content-Type: application/json" -d '{"action":"stop"}'

# Leinwand auf 50% positionieren
curl -X POST http://<adapter-ip>/api/screen -H "Content-Type: application/json" -d '{"action":"position","pos":50}'

# Status abfragen
curl http://<adapter-ip>/api/screen/status
```

---

## Szenario 6: Teilfehler — ein Shelly nicht erreichbar

Wenn ein Shelly nicht erreichbar ist, läuft die Szene trotzdem durch:

```json
{
  "scene": "start",
  "success": false,
  "steps": [
    { "device": "screen",  "action": "close",    "success": false, "error": "shelly_screen_unreachable" },
    { "device": "light",   "action": "off",      "success": true,  "error": "" },
    { "device": "beamer",  "action": "power_on", "success": true,  "error": "" },
    { "device": "beamer",  "action": "hdmi",     "success": true,  "error": "" }
  ]
}
```

`success=false` zeigt an, dass mindestens ein Schritt fehlschlug. HTTP-Status ist trotzdem 200.

---

## Web-UI: Neue Buttons

Nach dem Implementieren erscheinen in der Web-Oberfläche (`http://<adapter-ip>/`):

```
┌─────────────────────────────────────┐
│  Szenen                             │
│  ┌────────────────┐ ┌─────────────┐ │
│  │ Präsentation   │ │Präsentation │ │
│  │    starten     │ │  beenden    │ │
│  └────────────────┘ └─────────────┘ │
├─────────────────────────────────────┤
│  Licht                              │
│  ┌──────────┐ ┌──────────┐         │
│  │  Licht   │ │  Licht   │         │
│  │   ein    │ │   aus    │         │
│  └──────────┘ └──────────┘         │
├─────────────────────────────────────┤
│  Leinwand                           │
│  ┌────────┐ ┌────────┐ ┌────────┐  │
│  │ Runter │ │  Stop  │ │  Hoch  │  │
│  └────────┘ └────────┘ └────────┘  │
└─────────────────────────────────────┘
```
