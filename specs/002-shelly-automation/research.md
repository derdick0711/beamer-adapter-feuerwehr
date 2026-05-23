# Research: Shelly-Integration & Präsentationsautomatisierung

**Feature**: 002-shelly-automation
**Date**: 2026-05-23

---

## Shelly Gen3 HTTP RPC API

### Decision
Use the Shelly Gen3 local HTTP RPC API for all device control.

### Rationale
Shelly Gen3 devices expose a local REST-style RPC interface on the device IP address.
No cloud connection, no external dependencies, no auth required by default. The API
is identical in structure to Shelly Gen2 (same RPC method names, same JSON schemas),
so Gen2 documentation also applies to Gen3.

### Base URL Pattern

```
http://{device-ip}/rpc/{method}
```

Requests are sent as HTTP GET (parameters as query string) or HTTP POST (JSON body).
GET with query parameters is simpler for the ESP32 `HTTPClient` pattern and is preferred.

### Shelly 1 Mini Gen3 — Switch Control (Deckenlicht)

| Action       | HTTP Request                                        | Response (success)              |
|--------------|----------------------------------------------------|---------------------------------|
| Licht ein    | `GET /rpc/Switch.Set?id=0&on=true`                | `{"was_on": false}`             |
| Licht aus    | `GET /rpc/Switch.Set?id=0&on=false`               | `{"was_on": true}`              |
| Status       | `GET /rpc/Switch.GetStatus?id=0`                  | `{"id":0,"output":true,...}`    |

Relevant response fields from `Switch.GetStatus`:
- `output` (bool): current on/off state

### Shelly 2PM Gen3 — Cover/Roller Control (Leinwand)

The Shelly 2PM Gen3 must be pre-configured in **Roller/Cover mode** via the Shelly app.
When in roller mode, the Cover RPC component is active.

| Action           | HTTP Request                                            | Response (success)                |
|------------------|---------------------------------------------------------|-----------------------------------|
| Leinwand runter  | `GET /rpc/Cover.Close?id=0`                            | `{}`                              |
| Leinwand hoch    | `GET /rpc/Cover.Open?id=0`                             | `{}`                              |
| Stop             | `GET /rpc/Cover.Stop?id=0`                             | `{}`                              |
| Position (0–100%)| `GET /rpc/Cover.GoToPosition?id=0&pos={0-100}`         | `{}`                              |
| Status           | `GET /rpc/Cover.GetStatus?id=0`                        | `{"id":0,"state":"...","current_pos":75,...}` |

Relevant response fields from `Cover.GetStatus`:
- `state` (string): `"open"`, `"closed"`, `"opening"`, `"closing"`, `"stopped"`, `"calibrating"`
- `current_pos` (int, 0–100): 0 = vollständig geschlossen (Leinwand oben), 100 = vollständig offen (Leinwand unten)

**Shelly Positionskonvention** (Cover): `pos=100` = vollständig ausgefahren (Leinwand unten),
`pos=0` = vollständig eingefahren (Leinwand oben). Diese Konvention ist in der Spec bereits
als Annahme dokumentiert.

### Authentication
Shelly Gen3 devices have no authentication enabled by default on local networks.
Authentication (digest HTTP auth) can be set in device settings but is optional.
The ESP32 implementation assumes no auth. If auth is needed later, `HTTPClient::setAuthorization()`
can be added without changing the API structure.

### HTTP Timeout
Shelly devices on the same LAN respond within 50–200 ms. A 3-second HTTP timeout per
request is generous and safe. If a device does not respond within 3 s, it is flagged
as `reachable=false` and the sequence continues to the next step (FR-009).

---

## Admin-Passwort-Hashing

### Decision
SHA-256 Hash, gespeichert in NVS. Beim Login wird das eingegebene Passwort im Browser
mit SHA-256 gehasht (via SubtleCrypto API) und mit dem gespeicherten Hash verglichen.
Kein Session-Token-System — stattdessen HTTP Basic Auth über ESPAsyncWebServer für
einfache Integration.

### Rationale
Das Projekt hat einen einzigen Nutzer (Techniker), keinen Benutzermanagement-Overhead
und kein TLS. Für diese Anforderung ist HTTP Basic Auth (mit SHA-256 Passwort-Vergleich
im Handler) ausreichend und ESP32-kompatibel. Ein echter Session-Token wäre Overengineering.

### Implementation
- ESP32 speichert SHA-256-Hash des Admin-Passworts in NVS (`admin_pw_hash`)
- `/admin`-Route prüft HTTP Basic Auth (Username beliebig, Passwort wird gehashed und verglichen)
- Default-Passwort `feuerwehr` wird beim ersten Start gesetzt, wenn kein Hash in NVS vorhanden
- ESPAsyncWebServer: `request->authenticate()` / `request->requestAuthentication()`

---

## Sequenz-Timing

### Decision
Fire-and-forget pro Shelly-Schritt, sequenziell. Kein Delay zwischen Schritten,
außer vor dem Beamer-RS232-Befehl (500 ms Delay nach Shelly-Befehlen, damit keine
Überlast auf dem Netzwerk). RS232-Befehle mit ACK-Wartezeit (wie in Feature 001).

### Rationale
Laut Spec-Annahmen: "nächste Schritt startet, sobald der aktuelle Befehl abgeschickt
wurde (Fire-and-forget pro Schritt)". Die HTTP-Requests an Shelly sind synchron
blockierend (HTTPClient auf ESP32 ist blocking). Die Sequenz läuft daher automatisch
sequenziell durch, mit der Netzwerk-RTT als natürlichem Delay (~100–200 ms).

### "Präsentation starten" Sequenz
1. `Cover.Close` → Shelly 2PM Gen3 (Leinwand runter) — blocking HTTP GET (~200 ms)
2. `Switch.Set?on=false` → Shelly 1 Mini Gen3 (Licht aus) — blocking HTTP GET (~200 ms)
3. Power ON → BeamerRS232 (Beamer ein) — 500 ms ACK wait
4. Input HDMI → BeamerRS232 (HDMI Eingang) — 500 ms ACK wait

### "Präsentation beenden" Sequenz
1. `Cover.Open` → Shelly 2PM Gen3 (Leinwand hoch) — blocking HTTP GET (~200 ms)
2. `Switch.Set?on=true` → Shelly 1 Mini Gen3 (Licht ein) — blocking HTTP GET (~200 ms)
3. Power OFF → BeamerRS232 (Beamer aus) — 500 ms ACK wait

**Gesamtdauer**: Ca. 2–3 s (well within SC-001/SC-002 limit of 15 s).

---

## ESP32 HTTP Client Library

### Decision
`HTTPClient` aus dem Arduino-ESP32 Core (built-in, kein extra lib_deps-Eintrag nötig).

### Rationale
Bereits im Projekt verfügbar. Unterstützt GET/POST, Timeout-Konfiguration, und
funktioniert in Arduino-Task-Kontext. Für die synchronen Shelly-Aufrufe (max 5 Requests
pro Sequenz) ist blocking I/O akzeptabel, da die Sequenz sowieso serialisiert werden muss.

### HTTPClient Usage Pattern
```cpp
HTTPClient http;
http.begin("http://" + ip + "/rpc/Switch.Set?id=0&on=true");
http.setTimeout(3000);
int code = http.GET();
String body = http.getString();
http.end();
bool success = (code == 200);
```

---

## Alternatives Considered

| Alternative | Rejected Because |
|-------------|-----------------|
| Shelly MQTT (local) | Erfordert separaten MQTT Broker, höhere Latenz, more moving parts |
| Shelly WebSocket | Komplexer als HTTP GET für fire-and-forget commands |
| Shelly CoAP | Gen3 unterstützt CoAP nicht mehr; nur Gen1 |
| mDNS statt statischer IP | Unzuverlässig in einigen WLAN-Infrastrukturen, statische IP bevorzugt |
