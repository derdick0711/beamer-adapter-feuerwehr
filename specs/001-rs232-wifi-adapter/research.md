# Research: RS232-WiFi Beamer Adapter

**Feature**: 001-rs232-wifi-adapter
**Date**: 2026-05-22

---

## 1. Acer H6512BD RS232 Protocol

### Decision
9600 baud, 8N1, no flow control. Two overlapping command formats found in H-series
documentation; both are used (IR-format for power, C-format for inputs).

### Serial Parameters

| Parameter   | Value  |
|-------------|--------|
| Baud rate   | 9600   |
| Data bits   | 8      |
| Stop bits   | 1      |
| Parity      | None   |
| Flow control| None   |

### Command Set

All commands are terminated with CR (`\r`, 0x0D).

| Function        | Command string          | Notes                              |
|-----------------|-------------------------|------------------------------------|
| Power ON        | `*0 IR 001\r`           | Confirmed across H-series          |
| Power OFF       | `*0 IR 002\r`           | Confirmed across H-series          |
| Blank ON        | `*0 IR 055\r`           | Mute/blank — verify on hardware    |
| Blank OFF       | `*0 IR 055\r`           | Toggle; send twice if needed       |
| Input: HDMI     | `C36\r`                 | H-series confirmed                 |
| Input: VGA      | `C05\r`                 | VGA1/D-Sub                         |
| Input: Component| `C33\r`                 | YPbPr                              |
| Input: S-Video  | `C34\r`                 | S-Video                            |
| Input: Composite| `C35\r`                 | Composite — verify on hardware     |
| Status query    | `*0 IR 001?\r`          | Protocol variant — verify on HW    |

### Response Format

| Response | Bytes       | Meaning              |
|----------|-------------|----------------------|
| ACK      | 0x06 0x0D   | Command accepted     |
| NAK      | 0x15 0x0D   | Command rejected/error|

### Timing

- Inter-command delay: 50–100 ms recommended
- Response timeout: 500 ms before declaring RS232 failure (→ HTTP 503 per spec)

### Rationale
Standard 9600/8N1 is well-documented for the entire Acer H-series. Commands need
hardware-in-the-loop (HiL) verification for Blank and Status variants.

### Alternatives Considered
- HDMI-CEC: Not available via MAX3232 RS232 path — excluded by constitution.
- IR blasting: Excluded by constitution (RS232 only).

### Open Items (resolve during HiL testing)
- Confirm exact Blank ON/OFF command (toggle vs. separate codes).
- Confirm status query format and response payload.
- Confirm Composite input code (C35 vs. other).

---

## 2. PlatformIO Library Selection

### Decision
Four libraries chosen; all available in PlatformIO registry with stable versions.

| Role                  | Library                    | PIO ID                             | Rationale                         |
|-----------------------|----------------------------|------------------------------------|-----------------------------------|
| WiFi config (AP mode) | WiFiManager by tzapu       | `tzapu/WiFiManager`                | De-facto standard; captive portal; SSID scan; custom params for static IP + MQTT |
| HTTP/WebSocket server | ESPAsyncWebServer          | `me-no-dev/ESPAsyncWebServer`      | Async, low memory overhead, serves static files from SPIFFS/LittleFS |
| MQTT client           | PubSubClient by knolleary  | `knolleary/PubSubClient`           | Minimal, stable, widely used; fits embedded constraints |
| JSON serialization    | ArduinoJson                | `bblanchon/ArduinoJson`            | Zero-copy deserialization; stack-allocated docs; v7 recommended |

### Supporting Libraries (auto-pulled as dependencies)

- `me-no-dev/AsyncTCP` — required by ESPAsyncWebServer
- Built-in `Preferences` (Arduino-ESP32) — NVS key-value storage
- Built-in `HardwareSerial` — hardware UART for RS232

### PlatformIO Board & Platform

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.filesystem = littlefs
```

### Rationale
- WiFiManager covers AP mode, SSID scan, captive portal, and custom config fields
  (static IP, MQTT broker) in one library — avoids reimplementing from scratch.
- ESPAsyncWebServer is non-blocking; avoids watchdog issues during concurrent REST+MQTT.
- PubSubClient is lean (no async overhead) and sufficient for low-throughput MQTT.
- ArduinoJson v7 uses stack allocation for small documents; matches Principle IV.

### Alternatives Considered
- IotWebConf: More opinionated, less flexible for custom params.
- AsyncMqttClient: More capable but adds complexity not needed here (Principle V).
- LittleFS vs. SPIFFS: LittleFS chosen — better wear leveling, recommended for ESP32.

---

## 3. Static IP & NVS Configuration

### Decision
WiFiManager's `custom parameters` feature stores MQTT broker, static IP settings,
and RS232 baud rate in NVS via `Preferences`. Values survive power cycles.

### Parameters stored in NVS

| Key           | Type   | Default      | Description                  |
|---------------|--------|--------------|------------------------------|
| `wifi_ssid`   | string | (empty)      | Target WLAN SSID             |
| `wifi_pass`   | string | (empty)      | Target WLAN password         |
| `ip_static`   | bool   | false        | Enable static IP             |
| `ip_addr`     | string | (empty)      | Static IP address            |
| `ip_gw`       | string | (empty)      | Gateway                      |
| `ip_subnet`   | string | 255.255.255.0| Subnet mask                  |
| `mqtt_host`   | string | (empty)      | MQTT broker hostname/IP      |
| `mqtt_port`   | int    | 1883         | MQTT broker port             |
| `mqtt_prefix` | string | beamer       | MQTT topic prefix            |
| `rs232_baud`  | int    | 9600         | RS232 baud rate (override)   |

### Reset Procedure
GPIO0 (BOOT button on ESP32-S3 DevKit) held LOW for 5 s → clears NVS WLAN
credentials → restarts in AP mode.
