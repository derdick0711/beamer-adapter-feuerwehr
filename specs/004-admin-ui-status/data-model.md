# Data Model: Admin UI Status & Configuration Improvements

## Existing Entities (unchanged structure)

### NetworkConfig (`src/ConfigManager.h`)
Already contains all required fields — no structural change:
- `ssid` (String) — WiFi network name
- `password` (String) — WiFi passphrase (stored, not displayed in cleartext in UI)
- `staticIp` (bool) — whether static IP is active
- `ipAddr` (String) — static IP address
- `gateway` (String) — gateway address
- `subnet` (String) — subnet mask (default "255.255.255.0")

### ShellyConfig, MqttConfig, Rs232Config — no structural changes

## Runtime State (not persisted)

### MQTT Connection State
- Source: `MqttManager::isConnected()` → `PubSubClient::connected()`
- Included in `GET /api/status` response as `mqttConnected` (bool)
- Not stored; derived at request time

### Current IP
- Source: `WiFi.localIP().toString()`
- Included in `GET /api/status` response as `ip` (String)
- Also in `GET /admin/config` response
- Not stored; derived at request time

## MQTT Topics (additions)

| Topic | Direction | Retained | Payload |
|-------|-----------|----------|---------|
| `<prefix>/ip` | Device → Broker | Yes | IP address string (e.g., `192.168.178.249`) |

Published on every successful MQTT connect inside `MqttManager::_reconnect()`.
