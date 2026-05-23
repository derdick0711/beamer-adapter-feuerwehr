#include "RestHandler.h"
#include <ArduinoJson.h>

RestHandler gRest;

// ── helpers ──────────────────────────────────────────────────────────────────

static String powerStateName(PowerState s) {
    switch (s) {
        case PowerState::ON:  return "on";
        case PowerState::OFF: return "off";
        default:              return "unknown";
    }
}

void RestHandler::_sendStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["power"]       = powerStateName(gBeamerStatus.power);
    doc["input"]       = BeamerRS232::inputName(gBeamerStatus.input);
    doc["blank"]       = gBeamerStatus.blank;
    doc["reachable"]   = gBeamerStatus.reachable;
    doc["lastUpdated"] = gBeamerStatus.lastUpdated;
    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

void RestHandler::_sendError(AsyncWebServerRequest* req, int code,
                              const char* error, const char* field) {
    JsonDocument doc;
    doc["error"] = error;
    if (field) doc["field"] = field;
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

void RestHandler::_sendRS232Timeout(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["error"] = "rs232_timeout";
    JsonObject last = doc["lastKnown"].to<JsonObject>();
    last["power"] = powerStateName(gBeamerStatus.power);
    last["input"] = BeamerRS232::inputName(gBeamerStatus.input);
    last["blank"] = gBeamerStatus.blank;
    String body;
    serializeJson(doc, body);
    req->send(503, "application/json", body);
}

// ── route registration ────────────────────────────────────────────────────────

void RestHandler::begin(AsyncWebServer& server) {

    // GET /api/status
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        _sendStatus(req);
    });

    // GET /api/config
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["staticIp"]   = gConfig.network.staticIp;
        doc["ipAddr"]     = gConfig.network.ipAddr;
        doc["mqttHost"]   = gConfig.mqtt.host;
        doc["mqttPort"]   = gConfig.mqtt.port;
        doc["mqttPrefix"] = gConfig.mqtt.prefix;
        doc["lightIp"]    = gConfig.shelly.lightIp;
        doc["screenIp"]   = gConfig.shelly.screenIp;
        String body;
        serializeJson(doc, body);
        req->send(200, "application/json", body);
    });

    // POST /api/power  {"state":"on"|"off"}
    server.on("/api/power", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                _sendError(req, 400, "invalid_json");
                return;
            }
            if (!doc["state"].is<const char*>()) {
                _sendError(req, 400, "missing_field", "state");
                return;
            }
            String state = doc["state"].as<String>();
            BeamerCmd cmd;
            if (state == "on")       cmd = BeamerCmd::POWER_ON;
            else if (state == "off") cmd = BeamerCmd::POWER_OFF;
            else {
                _sendError(req, 400, "invalid_value", "state");
                return;
            }
            if (!gRS232.sendCommand(cmd)) { _sendRS232Timeout(req); return; }
            _sendStatus(req);
        });

    // POST /api/input  {"input":"hdmi"|"vga"|...}
    server.on("/api/input", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                _sendError(req, 400, "invalid_json");
                return;
            }
            if (!doc["input"].is<const char*>()) {
                _sendError(req, 400, "missing_field", "input");
                return;
            }
            String inputStr = doc["input"].as<String>();
            InputSource src = BeamerRS232::inputFromString(inputStr);
            if (src != InputSource::HDMI && src != InputSource::VGA) {
                _sendError(req, 400, "invalid_value", "input");
                return;
            }
            BeamerCmd cmd;
            switch (src) {
                case InputSource::HDMI: cmd = BeamerCmd::INPUT_HDMI; break;
                case InputSource::VGA:  cmd = BeamerCmd::INPUT_VGA;  break;
                default: _sendError(req, 400, "invalid_value", "input"); return;
            }
            if (!gRS232.sendCommand(cmd)) { _sendRS232Timeout(req); return; }
            _sendStatus(req);
        });

    // ── Shelly: Deckenlicht ──────────────────────────────────────────────────

    // GET /api/light/status
    server.on("/api/light/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        ShellyClient::getSwitchStatus(gLight);
        JsonDocument doc;
        doc["output"]    = gLight.output;
        doc["reachable"] = gLight.reachable;
        String body; serializeJson(doc, body);
        req->send(200, "application/json", body);
    });

    // POST /api/light  {"state":"on"|"off"}
    server.on("/api/light", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                _sendError(req, 400, "invalid_json"); return;
            }
            String state = doc["state"] | "";
            if (state != "on" && state != "off") {
                _sendError(req, 400, "invalid_state"); return;
            }
            ShellyResult r = ShellyClient::setSwitch(gLight, state == "on");
            if (!r.success) { _sendError(req, 503, "shelly_light_unreachable"); return; }
            JsonDocument out; out["ok"] = true;
            String body; serializeJson(out, body);
            req->send(200, "application/json", body);
        });

    // ── Shelly: Leinwand ────────────────────────────────────────────────────

    // GET /api/screen/status
    server.on("/api/screen/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        ShellyClient::getCoverStatus(gScreen);
        JsonDocument doc;
        doc["state"]       = gScreen.state;
        doc["current_pos"] = gScreen.currentPos;
        doc["reachable"]   = gScreen.reachable;
        String body; serializeJson(doc, body);
        req->send(200, "application/json", body);
    });

    // POST /api/screen  {"action":"open"|"close"|"stop"|"position", "pos":0-100}
    server.on("/api/screen", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                _sendError(req, 400, "invalid_json"); return;
            }
            String action = doc["action"] | "";
            ShellyResult r;
            if      (action == "open")     r = ShellyClient::coverOpen(gScreen);
            else if (action == "close")    r = ShellyClient::coverClose(gScreen);
            else if (action == "stop")     r = ShellyClient::coverStop(gScreen);
            else if (action == "position") {
                if (!doc["pos"].is<int>()) {
                    _sendError(req, 400, "pos required for action position"); return;
                }
                r = ShellyClient::coverGoToPosition(gScreen, doc["pos"].as<int>());
            } else {
                _sendError(req, 400, "invalid action"); return;
            }
            if (!r.success) { _sendError(req, 503, "shelly_screen_unreachable"); return; }
            JsonDocument out; out["ok"] = true;
            String body; serializeJson(out, body);
            req->send(200, "application/json", body);
        });

    // ── Szenen-Automatisierung ───────────────────────────────────────────────

    // POST /api/scene/start
    server.on("/api/scene/start", HTTP_POST, [](AsyncWebServerRequest* req) {
        SceneResult r = SceneManager::runStart();
        req->send(200, "application/json", r.toJson());
    });

    // POST /api/scene/stop
    server.on("/api/scene/stop", HTTP_POST, [](AsyncWebServerRequest* req) {
        SceneResult r = SceneManager::runStop();
        req->send(200, "application/json", r.toJson());
    });

    // POST /api/blank  {"enabled":true|false}
    server.on("/api/blank", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                _sendError(req, 400, "invalid_json");
                return;
            }
            if (!doc["enabled"].is<bool>()) {
                _sendError(req, 400, "missing_field", "enabled");
                return;
            }
            bool enable = doc["enabled"].as<bool>();
            BeamerCmd cmd = enable ? BeamerCmd::BLANK_ON : BeamerCmd::BLANK_OFF;
            if (!gRS232.sendCommand(cmd)) { _sendRS232Timeout(req); return; }
            _sendStatus(req);
        });
}
