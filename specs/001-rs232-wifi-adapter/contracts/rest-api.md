# REST API Contract: RS232-WiFi Beamer Adapter

**Base URL**: `http://{adapter-ip}`
**Content-Type**: `application/json`
**Auth**: None (anonymous)

---

## Endpoints

### GET /api/status

Returns the last known projector state.

**Response 200**:
```json
{
  "power": "on",
  "input": "hdmi",
  "blank": false,
  "reachable": true,
  "lastUpdated": 123456
}
```

**Response 503** (RS232 not responding):
```json
{
  "error": "rs232_timeout",
  "lastKnown": {
    "power": "on",
    "input": "hdmi",
    "blank": false
  }
}
```

---

### POST /api/power

Turn projector on or off.

**Request body**:
```json
{ "state": "on" }
```
`state`: `"on"` | `"off"`

**Response 200**:
```json
{ "power": "on", "reachable": true }
```

**Response 400** (invalid value):
```json
{ "error": "invalid_value", "field": "state", "allowed": ["on", "off"] }
```

**Response 503** (RS232 timeout):
```json
{ "error": "rs232_timeout" }
```

---

### POST /api/input

Select projector input source.

**Request body**:
```json
{ "input": "hdmi" }
```
`input`: `"hdmi"` | `"vga"` | `"component"` | `"svideo"` | `"composite"`

**Response 200**:
```json
{ "input": "hdmi", "reachable": true }
```

**Response 400**:
```json
{ "error": "invalid_value", "field": "input", "allowed": ["hdmi","vga","component","svideo","composite"] }
```

**Response 503**: see `/api/power`

---

### POST /api/blank

Enable or disable blank (black screen / mute).

**Request body**:
```json
{ "enabled": true }
```
`enabled`: `true` | `false`

**Response 200**:
```json
{ "blank": true, "reachable": true }
```

**Response 400**: see `/api/power`

**Response 503**: see `/api/power`

---

### GET /api/config

Returns current non-secret network and MQTT configuration.

**Response 200**:
```json
{
  "staticIp": false,
  "ipAddr": "",
  "mqttHost": "192.168.1.10",
  "mqttPort": 1883,
  "mqttPrefix": "beamer"
}
```

---

### GET /

Serves the built-in HTML control page (text/html).

---

## Error Codes Summary

| HTTP | `error` field     | Meaning                            |
|------|-------------------|------------------------------------|
| 400  | `invalid_value`   | Payload field has disallowed value |
| 400  | `missing_field`   | Required JSON field absent         |
| 503  | `rs232_timeout`   | Projector did not respond via RS232|
