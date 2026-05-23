#include "SceneManager.h"
#include "ShellyClient.h"
#include "BeamerRS232.h"
#include <ArduinoJson.h>

// ── SceneResult helpers ──────────────────────────────────────────────────────

static void addStep(SceneResult& r, const char* device, const char* action,
                    bool success, const String& error = "") {
    if (r.stepCount < 4) {
        r.steps[r.stepCount++] = {device, action, success, error};
        if (!success) r.success = false;
    }
}

String SceneResult::toJson() const {
    JsonDocument doc;
    doc["scene"]       = scene;
    doc["success"]     = success;
    doc["duration_ms"] = duration_ms;
    JsonArray arr = doc["steps"].to<JsonArray>();
    for (uint8_t i = 0; i < stepCount; i++) {
        JsonObject s = arr.add<JsonObject>();
        s["device"]  = steps[i].device;
        s["action"]  = steps[i].action;
        s["success"] = steps[i].success;
        s["error"]   = steps[i].error;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

// ── Scene: Präsentation starten ──────────────────────────────────────────────
// Sequence: (1) screen close → (2) light off → (3) beamer on → (4) hdmi

SceneResult SceneManager::runStart() {
    SceneResult r;
    r.scene   = "start";
    r.success = true;
    uint32_t t0 = millis();

    // Step 1: Leinwand runter
    ShellyResult sr = ShellyClient::coverClose(gScreen);
    addStep(r, "screen", "close", sr.success,
            sr.success ? "" : "shelly_screen_unreachable");

    // Step 2: Licht aus
    sr = ShellyClient::setSwitch(gLight, false);
    addStep(r, "light", "off", sr.success,
            sr.success ? "" : "shelly_light_unreachable");

    // Step 3: Beamer ein
    bool beamerOn = gRS232.sendCommand(BeamerCmd::POWER_ON);
    addStep(r, "beamer", "power_on", beamerOn,
            beamerOn ? "" : "rs232_timeout");

    // Step 4: HDMI Eingang
    bool hdmi = gRS232.sendCommand(BeamerCmd::INPUT_HDMI);
    addStep(r, "beamer", "hdmi", hdmi,
            hdmi ? "" : "rs232_timeout");

    r.duration_ms = millis() - t0;
    return r;
}

// ── Scene: Präsentation beenden ──────────────────────────────────────────────
// Sequence: (1) screen open → (2) light on → (3) beamer off

SceneResult SceneManager::runStop() {
    SceneResult r;
    r.scene   = "stop";
    r.success = true;
    uint32_t t0 = millis();

    // Step 1: Leinwand hoch
    ShellyResult sr = ShellyClient::coverOpen(gScreen);
    addStep(r, "screen", "open", sr.success,
            sr.success ? "" : "shelly_screen_unreachable");

    // Step 2: Licht ein
    sr = ShellyClient::setSwitch(gLight, true);
    addStep(r, "light", "on", sr.success,
            sr.success ? "" : "shelly_light_unreachable");

    // Step 3: Beamer aus
    bool beamerOff = gRS232.sendCommand(BeamerCmd::POWER_OFF);
    addStep(r, "beamer", "power_off", beamerOff,
            beamerOff ? "" : "rs232_timeout");

    r.duration_ms = millis() - t0;
    return r;
}
