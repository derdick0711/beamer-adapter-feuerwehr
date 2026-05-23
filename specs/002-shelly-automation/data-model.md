# Data Model: Shelly-Integration & Präsentationsautomatisierung

**Feature**: 002-shelly-automation
**Date**: 2026-05-23

---

## Entities

### ShellySwitchDevice

Repräsentiert den Shelly 1 Mini Gen3 (Deckenlicht).
In RAM gecacht; persistent sind nur IP-Adresse und Name in NVS.

| Field       | Type   | Values                  | Notes                              |
|-------------|--------|-------------------------|------------------------------------|
| `ip`        | String | IPv4-Adresse            | Aus NVS geladen; via Admin änderbar |
| `name`      | String | z.B. `"Deckenlicht"`    | Anzeigename, fest kodiert           |
| `output`    | bool   | `true` / `false`        | Letzter bekannter Schaltzustand     |
| `reachable` | bool   | `true` / `false`        | HTTP-Antwort innerhalb von 3 s      |

**State transitions**:
- `reachable=false` wenn HTTP-Request innerhalb von 3 s keinen HTTP 200 liefert
- `output` wird nach jedem `Switch.Set` oder `Switch.GetStatus` aktualisiert

---

### ShellyRollerDevice

Repräsentiert den Shelly 2PM Gen3 (Leinwand), konfiguriert im Roller/Cover-Modus.

| Field         | Type   | Values                                                              | Notes                              |
|---------------|--------|---------------------------------------------------------------------|------------------------------------|
| `ip`          | String | IPv4-Adresse                                                        | Aus NVS geladen; via Admin änderbar |
| `name`        | String | z.B. `"Leinwand"`                                                   | Anzeigename, fest kodiert           |
| `state`       | String | `"open"`, `"closed"`, `"opening"`, `"closing"`, `"stopped"`, `"unknown"` | Fahrzustand der Leinwand     |
| `currentPos`  | int    | 0–100                                                               | 0=oben (eingerollt), 100=unten (ausgerollt) |
| `reachable`   | bool   | `true` / `false`                                                    | HTTP-Antwort innerhalb von 3 s      |

**State transitions**:
- `state="unknown"` vor erster Statusabfrage
- `state` und `currentPos` werden nach `Cover.GetStatus` aktualisiert
- Positionskonvention: Shelly `pos=0` → Leinwand oben, `pos=100` → Leinwand unten

---

### SceneStep

Einzelner Schritt innerhalb einer Automatisierungssequenz. Nicht persistiert;
nur während der Sequenzausführung im Stack.

| Field     | Type   | Values                                  | Notes                            |
|-----------|--------|-----------------------------------------|----------------------------------|
| `device`  | String | `"light"`, `"screen"`, `"beamer"`       | Zielgerät                        |
| `action`  | String | `"on"`, `"off"`, `"open"`, `"close"`, `"power_on"`, `"power_off"`, `"hdmi"` | Ausgeführte Aktion |
| `success` | bool   | `true` / `false`                        | Ob der Schritt erfolgreich war   |
| `error`   | String | Fehlermeldung oder leer                 | Nur bei `success=false` gesetzt  |

---

### SceneResult

Ergebnis einer vollständigen Automatisierungssequenz. Als JSON-Response zurückgegeben.

| Field        | Type         | Values                       | Notes                              |
|--------------|--------------|------------------------------|------------------------------------|
| `scene`      | String       | `"start"`, `"stop"`         | Welche Szene ausgeführt wurde      |
| `success`    | bool         | `true` / `false`             | Alle Schritte erfolgreich          |
| `steps`      | SceneStep[]  | Array                        | Ergebnis jedes einzelnen Schritts  |
| `duration_ms`| int          | Millisekunden                | Gesamtdauer der Sequenz            |

**Validierungsregeln**:
- `success=true` nur wenn alle `steps[].success=true`
- `success=false` wenn mind. ein Schritt fehlschlug (Sequenz läuft trotzdem durch — FR-009)

---

### AdminConfig

Persistiert in NVS. Enthält alle über die Admin-Oberfläche konfigurierbaren Parameter.

| Field           | Type   | Values               | Notes                                      |
|-----------------|--------|----------------------|--------------------------------------------|
| `lightIp`       | String | IPv4-Adresse         | IP des Shelly 1 Mini Gen3                  |
| `screenIp`      | String | IPv4-Adresse         | IP des Shelly 2PM Gen3                     |
| `adminPwHash`   | String | SHA-256-Hex (64 Zeichen) | Hash des Admin-Passworts               |

**NVS-Keys** (Namespace `shelly`):
- `light_ip` → ShellySwitchDevice.ip
- `screen_ip` → ShellyRollerDevice.ip
- `admin_pw` → AdminConfig.adminPwHash

**Default-Werte** (beim ersten Start wenn NVS leer):
- `lightIp` = `"192.168.1.100"` (Platzhalter — muss via Admin gesetzt werden)
- `screenIp` = `"192.168.1.101"` (Platzhalter — muss via Admin gesetzt werden)
- `adminPwHash` = SHA-256(`"feuerwehr"`)

---

## Globale Singletons (RAM)

| Singleton         | Typ                  | Scope                         |
|-------------------|----------------------|-------------------------------|
| `gLight`          | ShellySwitchDevice   | IP aus NVS; Status cached     |
| `gScreen`         | ShellyRollerDevice   | IP aus NVS; Status cached     |
| `gShellyConfig`   | AdminConfig          | Geladen aus NVS beim Boot     |

---

## Shelly HTTP Response Mapping

### Switch.GetStatus → ShellySwitchDevice

```json
{
  "id": 0,
  "output": true,
  "voltage": 230.4,
  "current": 0.123,
  "apower": 28.3
}
```

Relevant: `output` (bool) → `ShellySwitchDevice.output`

### Cover.GetStatus → ShellyRollerDevice

```json
{
  "id": 0,
  "source": "limit_switch",
  "state": "open",
  "apower": 0.0,
  "current_pos": 100
}
```

Relevant:
- `state` (string) → `ShellyRollerDevice.state`
- `current_pos` (int, 0–100) → `ShellyRollerDevice.currentPos`
