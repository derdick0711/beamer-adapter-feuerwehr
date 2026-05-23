# REST API Contract: Shelly-Integration & Szenen-Automatisierung

**Feature**: 002-shelly-automation
**Date**: 2026-05-23
**Extends**: `specs/001-rs232-wifi-adapter/contracts/rest-api.md`

---

## Neue Endpunkte

### GET /api/light/status

Gibt den aktuellen Schaltzustand des Deckenlichts zurück.

**Response 200**:
```json
{
  "output": true,
  "reachable": true
}
```

| Field       | Type | Description                                  |
|-------------|------|----------------------------------------------|
| `output`    | bool | `true` = Licht an, `false` = Licht aus       |
| `reachable` | bool | `false` wenn Shelly 1 Mini Gen3 nicht antwortet |

---

### POST /api/light

Schaltet das Deckenlicht ein oder aus.

**Request Body**:
```json
{ "state": "on" }
```

| Field   | Type   | Values          | Required |
|---------|--------|-----------------|----------|
| `state` | string | `"on"`, `"off"` | ja       |

**Response 200**:
```json
{ "ok": true }
```

**Response 400** (ungültiger `state`):
```json
{ "error": "invalid state" }
```

**Response 503** (Shelly nicht erreichbar):
```json
{ "error": "shelly_light_unreachable" }
```

---

### GET /api/screen/status

Gibt den aktuellen Fahrzustand der Leinwand zurück.

**Response 200**:
```json
{
  "state": "open",
  "current_pos": 100,
  "reachable": true
}
```

| Field         | Type   | Description                                         |
|---------------|--------|-----------------------------------------------------|
| `state`       | string | `"open"`, `"closed"`, `"opening"`, `"closing"`, `"stopped"`, `"unknown"` |
| `current_pos` | int    | 0 = oben (eingerollt), 100 = unten (ausgerollt)    |
| `reachable`   | bool   | `false` wenn Shelly 2PM Gen3 nicht antwortet        |

---

### POST /api/screen

Steuert die Leinwand.

**Request Body**:
```json
{ "action": "close" }
```

Für Positionierung:
```json
{ "action": "position", "pos": 75 }
```

| Field    | Type   | Values                                      | Required |
|----------|--------|---------------------------------------------|----------|
| `action` | string | `"open"`, `"close"`, `"stop"`, `"position"` | ja       |
| `pos`    | int    | 0–100 (nur bei `action="position"`)         | nein     |

**Response 200**:
```json
{ "ok": true }
```

**Response 400**:
```json
{ "error": "invalid action" }
```
```json
{ "error": "pos required for action position" }
```

**Response 503** (Shelly nicht erreichbar):
```json
{ "error": "shelly_screen_unreachable" }
```

---

### POST /api/scene/start

Startet die Präsentations-Automatisierung:
(1) Leinwand runter → (2) Licht aus → (3) Beamer ein → (4) HDMI-Eingang

**Request Body**: kein Body erforderlich

**Response 200**:
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

**Partial Failure (200)**:
```json
{
  "scene": "start",
  "success": false,
  "duration_ms": 3521,
  "steps": [
    { "device": "screen",  "action": "close",    "success": false, "error": "shelly_screen_unreachable" },
    { "device": "light",   "action": "off",      "success": true,  "error": "" },
    { "device": "beamer",  "action": "power_on", "success": true,  "error": "" },
    { "device": "beamer",  "action": "hdmi",     "success": true,  "error": "" }
  ]
}
```

HTTP-Statuscode ist immer 200. `success=false` zeigt Teilfehler an.

---

### POST /api/scene/stop

Beendet die Präsentation:
(1) Leinwand hoch → (2) Licht ein → (3) Beamer aus

**Request Body**: kein Body erforderlich

**Response 200**:
```json
{
  "scene": "stop",
  "success": true,
  "duration_ms": 1234,
  "steps": [
    { "device": "screen",  "action": "open",      "success": true, "error": "" },
    { "device": "light",   "action": "on",        "success": true, "error": "" },
    { "device": "beamer",  "action": "power_off", "success": true, "error": "" }
  ]
}
```

---

### GET /admin

Liefert die Admin-Konfigurationsseite (HTML). Passwortgeschützt via HTTP Basic Auth.

**Authorization**: `Authorization: Basic <base64(:<passwort>)>`

**Response 200**: HTML-Seite mit Formular (Shelly-IPs, Passwort-Änderung)

**Response 401**: Wenn kein oder falsches Passwort angegeben wurde
```
WWW-Authenticate: Basic realm="Beamer Adapter Admin"
```

---

### POST /admin/save

Speichert neue Konfigurationswerte. Passwortgeschützt via HTTP Basic Auth.

**Request Body** (application/x-www-form-urlencoded):
```
light_ip=192.168.1.100&screen_ip=192.168.1.101&new_password=
```

| Field          | Required | Description                                        |
|----------------|----------|----------------------------------------------------|
| `light_ip`     | ja       | Neue IP des Shelly 1 Mini Gen3                     |
| `screen_ip`    | ja       | Neue IP des Shelly 2PM Gen3                        |
| `new_password` | nein     | Neues Admin-Passwort (leer = kein Wechsel)         |

**Response 303** (Redirect nach `/admin` bei Erfolg)

**Response 400** (ungültige IP-Adresse):
```json
{ "error": "invalid ip address" }
```

**Response 401**: Fehlende / falsche Authentifizierung
