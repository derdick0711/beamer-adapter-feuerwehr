#include "OtaUpdater.h"
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

OtaUpdater gOta;

void OtaUpdater::begin(const char* owner, const char* repo) {
    _owner     = owner;
    _repo      = repo;
    _lastCheck = millis();  // first check after one full interval
    Serial.printf("[OTA] version=%s  repo=%s/%s\n", FIRMWARE_VERSION, owner, repo);
}

void OtaUpdater::loop() {
    if (millis() - _lastCheck < CHECK_INTERVAL_MS) return;
    _lastCheck = millis();
    _checkAndUpdate();
}

// ── Version comparison (semver, strips leading "v") ───────────────────────────

static bool parseVer(const String& s, int& maj, int& min, int& pat) {
    String v = s.startsWith("v") ? s.substring(1) : s;
    // Ignore pre-release suffix (e.g. "1.0.3-5-gabcdef") — only compare x.y.z
    int dash = v.indexOf('-');
    if (dash > 0) v = v.substring(0, dash);
    return sscanf(v.c_str(), "%d.%d.%d", &maj, &min, &pat) == 3;
}

bool OtaUpdater::_newerVersion(const String& latest) const {
    int lMaj, lMin, lPat, cMaj, cMin, cPat;
    if (!parseVer(latest, lMaj, lMin, lPat)) return false;
    if (!parseVer(String(FIRMWARE_VERSION), cMaj, cMin, cPat)) return false;
    if (lMaj != cMaj) return lMaj > cMaj;
    if (lMin != cMin) return lMin > cMin;
    return lPat > cPat;
}

// ── Main update check ─────────────────────────────────────────────────────────

void OtaUpdater::_checkAndUpdate() {
    Serial.println("[OTA] Checking GitHub for update...");

    // ── Step 1: fetch latest release metadata ──────────────────────────────
    WiFiClientSecure apiClient;
    apiClient.setInsecure();  // no cert pinning — acceptable for internal use

    HTTPClient http;
    String apiUrl = String("https://api.github.com/repos/")
                    + _owner + "/" + _repo + "/releases/latest";
    http.begin(apiClient, apiUrl);
    http.addHeader("User-Agent",  "ESP8266-BeamerAdapter");
    http.addHeader("Accept",      "application/vnd.github+json");

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[OTA] API error: %d\n", code);
        http.end();
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        Serial.printf("[OTA] JSON error: %s\n", err.c_str());
        return;
    }

    String latestTag  = doc["tag_name"] | "";
    String downloadUrl;
    for (JsonObject asset : doc["assets"].as<JsonArray>()) {
        if (String(asset["name"] | "") == "firmware.bin") {
            downloadUrl = asset["browser_download_url"] | "";
            break;
        }
    }

    if (latestTag.isEmpty() || downloadUrl.isEmpty()) {
        Serial.println("[OTA] No firmware.bin asset in latest release");
        return;
    }

    Serial.printf("[OTA] latest=%s  current=%s\n",
                  latestTag.c_str(), FIRMWARE_VERSION);

    if (!_newerVersion(latestTag)) {
        Serial.println("[OTA] Already up to date");
        return;
    }

    // ── Step 2: download and flash ─────────────────────────────────────────
    Serial.printf("[OTA] Flashing %s from %s\n",
                  latestTag.c_str(), downloadUrl.c_str());

    WiFiClientSecure flashClient;
    flashClient.setInsecure();

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);  // blink built-in LED during flash
    ESPhttpUpdate.rebootOnUpdate(true);

    t_httpUpdate_return ret = ESPhttpUpdate.update(flashClient, downloadUrl);

    // rebootOnUpdate=true means we only reach here on failure
    Serial.printf("[OTA] Update failed: %s\n",
                  ESPhttpUpdate.getLastErrorString().c_str());
}
