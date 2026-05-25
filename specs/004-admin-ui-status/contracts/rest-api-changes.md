# REST API Contract Changes

## GET /api/status — Extended Response

Adds `ip` and `mqttConnected` to the existing response.

**Response (200)**:
```json
{
  "power": "on|off|unknown",
  "input": "hdmi|vga|unknown",
  "blank": false,
  "reachable": true,
  "lastUpdated": 12345,
  "ip": "192.168.178.249",
  "mqttConnected": false
}
```

New fields:
- `ip` — current WiFi IP address string; `"0.0.0.0"` if not connected
- `mqttConnected` — true if MQTT is enabled AND broker connection is active

---

## GET /admin/config — Extended Response

Adds network/WiFi fields. Existing fields unchanged.

**Response (200)** (auth required):
```json
{
  "lightIp":    "192.168.1.100",
  "screenIp":   "192.168.1.101",
  "adminUser":  "admin",
  "mqttEnabled": false,
  "mqttHost":   "",
  "mqttPort":   1883,
  "mqttPrefix": "beamer",
  "ssid":       "MyWiFi",
  "staticIp":   false,
  "ipAddr":     "",
  "gateway":    "",
  "subnet":     "255.255.255.0",
  "currentIp":  "192.168.178.249"
}
```

New fields: `ssid`, `staticIp`, `ipAddr`, `gateway`, `subnet`, `currentIp`

---

## POST /admin/save — Partial Save Semantics

All fields are now optional. Empty string = no change. MQTT enable toggle is the exception: if `mqtt_enabled` is not present in POST body, the stored value is preserved (not reset to false).

**Request** (form-encoded, auth required):

| Field | Type | Behaviour |
|-------|------|-----------|
| `light_ip` | string | Update only if non-empty AND valid IP |
| `screen_ip` | string | Update only if non-empty AND valid IP |
| `wifi_ssid` | string | Update only if non-empty |
| `wifi_password` | string | Update only if non-empty |
| `static_ip` | checkbox | Always processed (on = enable, absent = disable) |
| `ip_addr` | string | Update only if non-empty when static_ip is on |
| `ip_gw` | string | Update only if non-empty when static_ip is on |
| `ip_sub` | string | Update only if non-empty when static_ip is on |
| `new_username` | string | Update only if non-empty |
| `new_password` | string | Update only if non-empty |
| `mqtt_enabled` | checkbox | Always processed |
| `mqtt_host` | string | Update only if non-empty |
| `mqtt_port` | string | Update only if non-empty and valid number |
| `mqtt_prefix` | string | Update only if non-empty |

**Redirect on success**: 
- `/admin?wifi=1` if WiFi SSID or password changed (triggers restart banner)
- `/admin?mqtt=1` if MQTT settings changed (triggers restart banner)  
- `/admin` otherwise

---

## MQTT Contract Addition

| Topic | Payload | Retained | Published when |
|-------|---------|----------|----------------|
| `<prefix>/ip` | `"192.168.178.249"` | Yes | On every successful MQTT broker connect |
