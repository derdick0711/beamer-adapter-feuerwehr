# Implementation Plan: Admin UI Status & Configuration Improvements

**Branch**: `004-admin-ui-status` | **Date**: 2026-05-25 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/004-admin-ui-status/spec.md`

## Summary

Extend the admin page to pre-fill all stored settings, add WiFi/IP configuration editing, apply partial-save semantics (empty = no change), and show MQTT configuration only when active. Extend the main page to display the device's current IP and live MQTT connection status. Publish the device IP as a retained MQTT message on connect.

## Technical Context

**Language/Version**: C++17 (Arduino framework) + HTML/CSS/ES6 JS

**Primary Dependencies**: ESPAsyncWebServer, PubSubClient 2.8, ArduinoJson 7.x, LittleFS, ESP8266WiFi

**Storage**: LittleFS (`/config.json`) — existing schema, no structural changes

**Testing**: Hardware-in-the-loop against physical WeMos D1 Mini

**Target Platform**: ESP8266 WeMos D1 Mini (3.3 V, 4 MB Flash, 80 MHz)

**Performance Goals**: Page load < 3 s on local WiFi; MQTT IP publish within 5 s of broker connect

**Constraints**: No new heap-heavy allocations; all String operations bounded; LittleFS file writes only on explicit save

**Scale/Scope**: Single device, single user at a time

> **Note**: The constitution references ESP32-S3 as the MCU. The project migrated to ESP8266 WeMos D1 Mini (commit `7b59146`). All other constitutional principles apply unchanged.

## Constitution Check

| Principle | Status | Notes |
|-----------|--------|-------|
| I — RS232 Serial | PASS | No RS232 changes |
| II — WiFi Remote Control | PASS | Enhances WiFi control interface; credentials stored in config, not hardcoded |
| III — Reliability-First | PASS | Partial save is safer than full overwrite; restart-on-credential-change is the established safe pattern |
| IV — Embedded-Efficient Design | PASS | No new heap-heavy objects; String operations bounded; no dynamic arrays |
| V — Simplicity / Minimal Footprint | PASS | All additions have direct fire-station operational use cases |

## Project Structure

### Documentation (this feature)

```text
specs/004-admin-ui-status/
├── plan.md              ← this file
├── spec.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── rest-api-changes.md
├── checklists/
│   └── requirements.md
└── tasks.md             ← created by /speckit-tasks
```

### Source Files Changed

```text
src/
├── MqttManager.h          # add isConnected() public accessor
├── MqttManager.cpp        # publish <prefix>/ip retained on connect
├── RestHandler.cpp        # extend /api/status with ip + mqttConnected
├── AdminHandler.cpp       # partial save; WiFi fields; extended /admin/config
└── AdminHandler.h         # no change expected

data/
├── admin.html             # WiFi section; MQTT conditional display; pre-fill all fields
├── index.html             # IP badge; MQTT status badge
└── app.js                 # handle ip + mqttConnected in updateUI()
```

## Implementation Steps

### Step 1 — MqttManager: `isConnected()` + IP publish

**File**: `src/MqttManager.h`
- Add `bool isConnected() const;` to public interface

**File**: `src/MqttManager.cpp`
- Implement `isConnected()` returning `_client.connected()`
- In `_reconnect()`, after successful connect + subscribe, publish retained IP:
  ```cpp
  String ipTopic = _prefix + "/ip";
  String ipVal   = WiFi.localIP().toString();
  _client.publish(ipTopic.c_str(), ipVal.c_str(), true);
  ```

---

### Step 2 — RestHandler: Extend `/api/status`

**File**: `src/RestHandler.cpp` — `_sendStatus()`
- Add two fields to JSON response:
  ```cpp
  doc["ip"]           = WiFi.localIP().toString();
  doc["mqttConnected"] = gMqtt.isConnected();
  ```
- Add `#include <ESP8266WiFi.h>` if not already present

---

### Step 3 — AdminHandler: Extend `GET /admin/config`

**File**: `src/AdminHandler.cpp` — `/admin/config` handler
- Add to JSON response:
  ```cpp
  doc["ssid"]      = gConfig.network.ssid;
  doc["staticIp"]  = gConfig.network.staticIp;
  doc["ipAddr"]    = gConfig.network.ipAddr;
  doc["gateway"]   = gConfig.network.gateway;
  doc["subnet"]    = gConfig.network.subnet;
  doc["currentIp"] = WiFi.localIP().toString();
  ```

---

### Step 4 — AdminHandler: Partial Save in `POST /admin/save`

**File**: `src/AdminHandler.cpp` — `/admin/save` handler

Replace the current all-or-nothing save with field-by-field conditional updates:

**Shelly IPs** (partial save, validate only if non-empty):
```cpp
if (lightIp.length() > 0 && validIp(lightIp))  gConfig.shelly.lightIp  = lightIp;
if (screenIp.length() > 0 && validIp(screenIp)) gConfig.shelly.screenIp = screenIp;
```
Remove the 400 error that fires when both IPs fail validation — only reject an explicitly supplied invalid IP.

**WiFi credentials** (new):
```cpp
String wifiSsid = req->hasParam("wifi_ssid", true) ? req->getParam("wifi_ssid", true)->value() : "";
String wifiPass = req->hasParam("wifi_password", true) ? req->getParam("wifi_password", true)->value() : "";
bool wifiChanged = false;
if (wifiSsid.length() > 0) { gConfig.network.ssid     = wifiSsid;  wifiChanged = true; }
if (wifiPass.length() > 0) { gConfig.network.password = wifiPass;  wifiChanged = true; }
```

**Static IP** (new):
```cpp
bool staticOn = req->hasParam("static_ip", true);
gConfig.network.staticIp = staticOn;
if (staticOn) {
    String ipAddr = req->hasParam("ip_addr", true) ? req->getParam("ip_addr", true)->value() : "";
    String ipGw   = req->hasParam("ip_gw",   true) ? req->getParam("ip_gw",   true)->value() : "";
    String ipSub  = req->hasParam("ip_sub",   true) ? req->getParam("ip_sub",   true)->value() : "";
    if (ipAddr.length() > 0) gConfig.network.ipAddr   = ipAddr;
    if (ipGw.length()   > 0) gConfig.network.gateway  = ipGw;
    if (ipSub.length()  > 0) gConfig.network.subnet   = ipSub;
    wifiChanged = true;
}
gConfig.saveNetwork(gConfig.network);
```

**Redirect logic**:
```cpp
if (wifiChanged && mqttChanged) req->redirect("/admin?wifi=1&mqtt=1");
else if (wifiChanged)           req->redirect("/admin?wifi=1");
else if (mqttChanged)           req->redirect("/admin?mqtt=1");
else                            req->redirect("/admin");
```

---

### Step 5 — `data/admin.html`: WiFi section + conditional MQTT + pre-fill all fields

**WiFi section** (new card, insert before MQTT):
```html
<section class="card">
  <h2>WLAN</h2>
  <div class="form-group">
    <label for="wifi_ssid">SSID</label>
    <input type="text" id="wifi_ssid" name="wifi_ssid" placeholder="Leer lassen wenn unverändert">
  </div>
  <div class="form-group">
    <label for="wifi_password">Passwort <small>(leer = unverändert)</small></label>
    <input type="password" id="wifi_password" name="wifi_password" placeholder="Leer lassen wenn unverändert" autocomplete="new-password">
  </div>
  <div class="toggle-row">
    <label class="toggle">
      <input type="checkbox" id="static_ip" name="static_ip" onchange="toggleStaticIp(this.checked)">
      <span class="slider"></span>
    </label>
    <span>Feste IP</span>
  </div>
  <div id="static-ip-fields" class="hidden">
    <div class="form-group">
      <label for="ip_addr">IP-Adresse</label>
      <input type="text" id="ip_addr" name="ip_addr" placeholder="192.168.1.50">
    </div>
    <div class="form-group">
      <label for="ip_gw">Gateway</label>
      <input type="text" id="ip_gw" name="ip_gw" placeholder="192.168.1.1">
    </div>
    <div class="form-group">
      <label for="ip_sub">Subnetz</label>
      <input type="text" id="ip_sub" name="ip_sub" placeholder="255.255.255.0">
    </div>
  </div>
</section>
```

**Shelly section**: Add `<span class="status-dot" id="dot-light"></span>` and `<span class="status-dot" id="dot-screen"></span>` next to each IP label. Set `.on` class from JS when IP is non-empty.

**MQTT section**: Wrap existing MQTT fields in a div `id="mqtt-fields"`. Show/hide based on whether `mqttEnabled` or `mqttHost` is set. Always show the toggle.

**Restart banner**: Extend existing banner JS to also handle `?wifi=1` param:
```js
if (params.get('wifi') === '1') {
  document.getElementById('restart-banner').classList.add('visible');
}
```

**JS fetch `/admin/config`**: Extend to populate all new fields:
```js
document.getElementById('wifi_ssid').placeholder = d.ssid || 'Leer lassen wenn unverändert';
const si = d.staticIp || false;
document.getElementById('static_ip').checked = si;
toggleStaticIp(si);
if (d.ipAddr)   document.getElementById('ip_addr').value = d.ipAddr;
if (d.gateway)  document.getElementById('ip_gw').value   = d.gateway;
if (d.subnet)   document.getElementById('ip_sub').value  = d.subnet;
// Shelly dots
setDot('dot-light',  d.lightIp  && d.lightIp.length  > 0);
setDot('dot-screen', d.screenIp && d.screenIp.length > 0);
// MQTT: show fields only if configured
if (d.mqttEnabled || (d.mqttHost && d.mqttHost.length > 0)) {
    document.getElementById('mqtt-fields').classList.remove('hidden');
}
```

Helper functions:
```js
function toggleStaticIp(on) {
  document.getElementById('static-ip-fields').classList.toggle('hidden', !on);
}
function setDot(id, configured) {
  const el = document.getElementById(id);
  if (el) el.className = 'status-dot ' + (configured ? 'on' : 'off');
}
```

---

### Step 6 — `data/index.html`: IP and MQTT badges

Add to `#status-bar`:
```html
<span id="status-ip"   class="badge hidden">–</span>
<span id="status-mqtt" class="badge hidden">MQTT: OFF</span>
```

---

### Step 7 — `data/app.js`: Handle new status fields

In `updateUI(status)`:
```js
const ipBadge = document.getElementById('status-ip');
if (ipBadge && status.ip && status.ip !== '0.0.0.0') {
  ipBadge.textContent = status.ip;
  ipBadge.classList.remove('hidden');
}
const mqttBadge = document.getElementById('status-mqtt');
if (mqttBadge) {
  if (status.mqttConnected) {
    mqttBadge.textContent = 'MQTT: ON';
    mqttBadge.classList.remove('hidden');
  } else {
    mqttBadge.classList.add('hidden');
  }
}
```

---

## Deployment

1. `pio run --target upload --environment d1_mini` — firmware
2. `pio run --target uploadfs --environment d1_mini` — web UI (LittleFS)

Both required; both firmware (`src/`) and web files (`data/`) change.
