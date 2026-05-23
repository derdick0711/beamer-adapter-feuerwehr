# MQTT Contract: Shelly-Integration & Szenen-Automatisierung

**Feature**: 002-shelly-automation
**Date**: 2026-05-23
**Extends**: `specs/001-rs232-wifi-adapter/contracts/mqtt-contract.md`

Topic-Präfix `beamer` ist aus der NVS-Konfiguration (wie in Feature 001).

---

## Neue Command Topics (→ Adapter)

### `beamer/cmnd/light`

Schaltet das Deckenlicht ein oder aus.

| Payload  | Aktion              |
|----------|---------------------|
| `on`     | Deckenlicht ein     |
| `off`    | Deckenlicht aus     |

Ungültige Payloads werden ignoriert (kein Error-Topic).

---

### `beamer/cmnd/screen`

Steuert die Leinwand.

| Payload       | Aktion                           |
|---------------|----------------------------------|
| `open`        | Leinwand hoch                    |
| `close`       | Leinwand runter                  |
| `stop`        | Leinwand stoppen                 |
| `pos:{0-100}` | Leinwand auf Position 0–100 %    |

Beispiel Positionierung: `pos:75` → Leinwand auf 75 %

---

### `beamer/cmnd/scene/start`

Startet die Präsentations-Automatisierung (Leinwand runter → Licht aus → Beamer ein → HDMI).

| Payload | Bedeutung               |
|---------|-------------------------|
| `1`     | Szene starten           |
| (leer)  | Szene starten (auch ok) |

---

### `beamer/cmnd/scene/stop`

Beendet die Präsentation (Leinwand hoch → Licht ein → Beamer aus).

| Payload | Bedeutung               |
|---------|-------------------------|
| `1`     | Szene beenden           |
| (leer)  | Szene beenden (auch ok) |

---

## Neue Status Topics (← Adapter)

### `beamer/stat/light`

Wird nach jeder Licht-Aktion (Befehl oder Szene) veröffentlicht.
Retain: `true`

```json
{
  "output": true,
  "reachable": true
}
```

---

### `beamer/stat/screen`

Wird nach jeder Leinwand-Aktion veröffentlicht.
Retain: `true`

```json
{
  "state": "closing",
  "current_pos": 100,
  "reachable": true
}
```

---

### `beamer/stat/scene`

Wird nach Abschluss einer Szenensequenz (start oder stop) veröffentlicht.
Retain: `false`

```json
{
  "scene": "start",
  "success": true,
  "duration_ms": 1842,
  "steps": [
    { "device": "screen",  "action": "close",    "success": true,  "error": "" },
    { "device": "light",   "action": "off",      "success": true,  "error": "" },
    { "device": "beamer",  "action": "power_on", "success": true,  "error": "" },
    { "device": "beamer",  "action": "hdmi",     "success": true,  "error": "" }
  ]
}
```

---

## Topic-Übersicht (Feature 002 Ergänzungen)

| Topic                     | Richtung   | Retain | Beschreibung                   |
|---------------------------|------------|--------|--------------------------------|
| `beamer/cmnd/light`       | → Adapter  | nein   | Licht ein/aus                  |
| `beamer/cmnd/screen`      | → Adapter  | nein   | Leinwand auf/ab/stop/position  |
| `beamer/cmnd/scene/start` | → Adapter  | nein   | Präsentation starten           |
| `beamer/cmnd/scene/stop`  | → Adapter  | nein   | Präsentation beenden           |
| `beamer/stat/light`       | ← Adapter  | ja     | Licht-Status                   |
| `beamer/stat/screen`      | ← Adapter  | ja     | Leinwand-Status                |
| `beamer/stat/scene`       | ← Adapter  | nein   | Szenen-Ergebnis                |
