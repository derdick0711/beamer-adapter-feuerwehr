# Data Model: RS232-WiFi Beamer Adapter

**Feature**: 001-rs232-wifi-adapter
**Date**: 2026-05-22

---

## Entities

### BeamerStatus

Represents the last known state of the projector. Cached in RAM; updated after each
successful RS232 command or status query.

| Field         | Type    | Values                                         | Notes                           |
|---------------|---------|------------------------------------------------|---------------------------------|
| `power`       | enum    | `on`, `off`, `unknown`                         | `unknown` = never queried       |
| `input`       | enum    | `hdmi`, `vga`, `component`, `svideo`, `composite`, `unknown` | Active input    |
| `blank`       | bool    | `true` / `false`                               | Blank/mute active               |
| `reachable`   | bool    | `true` / `false`                               | Last RS232 response was ACK     |
| `lastUpdated` | uint32  | millis() timestamp                             | ms since boot of last update    |

**State transitions**:
- `unknown` → `on`/`off` after first successful status query or power command + ACK
- `reachable` = false when RS232 timeout occurs (→ HTTP 503 reported to caller)
- Blank does not change power state

---

### BeamerCommand

Represents a single control instruction sent to the projector. Not persisted;
constructed on demand from REST/MQTT/Web input.

| Field     | Type   | Values                                   |
|-----------|--------|------------------------------------------|
| `type`    | enum   | `power`, `input`, `blank`                |
| `value`   | string | `on`/`off`, input name, `true`/`false`   |

**Validation rules**:
- `type=input` → `value` MUST be one of: `hdmi`, `vga`, `component`, `svideo`, `composite`
- `type=power` → `value` MUST be `on` or `off`
- `type=blank` → `value` MUST be `true` or `false`
- Unknown `value` → reject with HTTP 400 / MQTT no-op

---

### NetworkConfig

Persisted to NVS via `Preferences`. Written by WiFiManager on successful config save.

| Field       | Type   | Required | Default        |
|-------------|--------|----------|----------------|
| `ssid`      | string | yes      | —              |
| `password`  | string | yes      | —              |
| `staticIp`  | bool   | no       | `false`        |
| `ipAddr`    | string | no       | —              |
| `gateway`   | string | no       | —              |
| `subnet`    | string | no       | `255.255.255.0`|

**Lifecycle**:
- Created/updated: WiFiManager configuration portal save event
- Deleted: Reset-button 5 s hold
- On delete: restart into AP mode

---

### MqttConfig

Persisted to NVS. Configurable via WiFiManager custom parameters.

| Field       | Type   | Required | Default  |
|-------------|--------|----------|----------|
| `host`      | string | yes      | —        |
| `port`      | uint16 | no       | `1883`   |
| `prefix`    | string | no       | `beamer` |

**Topic derivation** (runtime, not stored):
- Subscribe: `{prefix}/cmnd/power`, `{prefix}/cmnd/input`, `{prefix}/cmnd/blank`
- Publish:   `{prefix}/stat`
- LWT topic: `{prefix}/stat`, payload: `{"reachable":false,"reason":"lwt"}`

---

### Rs232Config

Persisted to NVS. Defaults match Acer H6512BD specification.

| Field       | Type   | Required | Default |
|-------------|--------|----------|---------|
| `baud`      | uint32 | no       | `9600`  |
| `rxPin`     | uint8  | no       | `16`    |
| `txPin`     | uint8  | no       | `17`    |

**Constraints**:
- `rxPin`/`txPin` MUST map to a hardware UART (UART1 or UART2) of the ESP32-S3.
- Software serial emulation is prohibited (Constitution §Hardware Constraints).

---

## State Machine: Boot Sequence

```
Power ON
    │
    ▼
NVS: SSID present?
    │ Yes                 No
    ▼                     ▼
Connect WLAN          AP Mode (5 min timeout)
    │                     │ Config saved
    │ Connected           ▼
    ▼                 Restart
Start MQTT client
Start HTTP server
Start RS232 UART
Ready (operational)
    │
    ├── RS232 Timeout → HTTP 503, keep last status, no restart
    ├── WLAN lost → auto-reconnect loop, MQTT reconnects after WLAN
    └── MQTT disconnected → auto-reconnect loop (independent of WLAN)
```
