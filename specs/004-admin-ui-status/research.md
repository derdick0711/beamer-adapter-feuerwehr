# Research: Admin UI Status & Configuration Improvements

## MQTT Retained Publish

**Decision**: Use PubSubClient's built-in retain flag.
**Rationale**: `_client.publish(topic.c_str(), payload.c_str(), true)` — third argument is `retained`. No additional library needed. Publish `<prefix>/ip` with device IP on every MQTT connect event inside `_reconnect()`.
**Alternatives considered**: Separate MQTT helper class — unnecessary given single publish point.

## WiFi Credential Change

**Decision**: Store new credentials via `ConfigManager::saveNetwork()`, show restart banner, apply on next boot.
**Rationale**: ESP8266 WiFi reconnection after `WiFi.begin(newSsid, newPass)` while a server is running is unreliable. Restart is the safe path; already used for MQTT changes.
**Alternatives considered**: Runtime reconnect via `WiFi.disconnect()` + `WiFi.begin()` — risks dropping the web server mid-response.

## Current IP Address

**Decision**: `WiFi.localIP().toString()` called at request time inside `/api/status` and admin config handlers.
**Rationale**: Already available via ESP8266WiFi; no storage needed; always reflects actual assigned address.
**Alternatives considered**: Store IP in ConfigManager — unnecessary, adds stale-data risk.

## MQTT Connected State Exposure

**Decision**: Add `bool isConnected() const` to `MqttManager` returning `_client.connected()`.
**Rationale**: PubSubClient's `connected()` is the canonical state. Expose it as a thin accessor rather than duplicating state.
**Alternatives considered**: Global bool flag — unnecessary coupling.

## Partial Save for Shelly IPs

**Decision**: Make `validIp()` validation conditional — only validate non-empty values; skip update if empty.
**Rationale**: Consistent with the clarified partial-save rule (empty = no change). Invalid non-empty IPs still rejected.
**Alternatives considered**: Keep IPs required — rejected per spec clarification Q1.

## Static IP Application

**Decision**: Store and show "Neustart erforderlich" banner; apply on next boot via `WiFi.config()` in `WifiProvisioner::begin()`.
**Rationale**: `WiFi.config()` must be called before `WiFi.begin()` which only happens at boot. Already handled by `WifiProvisioner`.
**Alternatives considered**: Apply immediately — not feasible while web server is running.
