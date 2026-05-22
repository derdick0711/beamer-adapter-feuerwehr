# MQTT Contract: RS232-WiFi Beamer Adapter

**Broker**: external (anonymous, no TLS)
**Default prefix**: `beamer` (configurable in NVS)
**QoS**: 0 for all topics
**Retain**: status topic published with retain=true

---

## Topics Overview

| Direction        | Topic                    | Payload       |
|------------------|--------------------------|---------------|
| Subscribe (cmnd) | `{prefix}/cmnd/power`    | `on` / `off`  |
| Subscribe (cmnd) | `{prefix}/cmnd/input`    | input name    |
| Subscribe (cmnd) | `{prefix}/cmnd/blank`    | `true` / `false` |
| Publish (stat)   | `{prefix}/stat`          | JSON object   |
| LWT              | `{prefix}/stat`          | JSON object   |

---

## Command Topics (adapter subscribes)

### `{prefix}/cmnd/power`

Payload: plain string `on` or `off`

Behaviour: Sends RS232 power command. Publishes updated status to `{prefix}/stat`
after ACK. On RS232 timeout: publishes error status, no retry.

### `{prefix}/cmnd/input`

Payload: plain string — one of `hdmi`, `vga`, `component`, `svideo`, `composite`

Behaviour: Sends RS232 input-select command. Publishes updated status to `{prefix}/stat`.
Unknown payload: ignored (no-op, no error published).

### `{prefix}/cmnd/blank`

Payload: plain string `true` or `false`

Behaviour: Sends RS232 blank command. Publishes updated status to `{prefix}/stat`.

---

## Status Topic (adapter publishes)

### `{prefix}/stat`

Published after every command execution and on startup. Retained.

**Normal payload**:
```json
{
  "power": "on",
  "input": "hdmi",
  "blank": false,
  "reachable": true,
  "uptime": 3600
}
```

**RS232 error payload**:
```json
{
  "reachable": false,
  "error": "rs232_timeout",
  "lastKnown": {
    "power": "on",
    "input": "hdmi",
    "blank": false
  }
}
```

**Last Will Testament (LWT)** — sent by broker when adapter disconnects unexpectedly:
```json
{
  "reachable": false,
  "reason": "lwt"
}
```

---

## Connection Behaviour

- Adapter connects on boot (after WLAN is established).
- On MQTT disconnect: exponential back-off reconnect (max 60 s interval).
- On WLAN reconnect: MQTT reconnect triggered automatically.
- Client ID: `beamer-adapter-{MAC_SUFFIX}` (unique per device).
