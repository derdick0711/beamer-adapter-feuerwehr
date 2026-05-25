# Quickstart: Admin UI Status & Configuration Improvements

## What Changes

### Main Page (`/`)
- Status bar shows current device IP address
- "MQTT: ON" badge appears when MQTT is enabled and connected (polled every 5 s)

### Admin Page (`/admin`)
- **Shelly section**: fields pre-filled; green dot when IP is configured
- **WiFi section**: editable SSID + masked password; static IP toggle with address fields
- **MQTT section**: hidden when not configured; expanded with values when configured
- **Saving**: empty fields are skipped (no unintended overwrites)
- **Restart banner**: shown after WiFi or MQTT changes

## Verifying After Deployment

1. Open `http://<device-ip>/` — IP badge visible in header status bar
2. If MQTT enabled and broker reachable — "MQTT: ON" badge appears
3. Open `http://<device-ip>/admin` (admin / feuerwehr) — all saved settings pre-filled
4. Subscribe to `<prefix>/ip` on MQTT broker — retained IP message present
5. Change only Shelly Light IP, save → only that IP updates; all other settings unchanged

## Upload Sequence

```
pio run --target upload --environment d1_mini     # firmware
pio run --target uploadfs --environment d1_mini   # web files (LittleFS)
```

Both uploads required since both firmware and `data/` files change.
