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
            if (src == InputSource::UNKNOWN) {
                _sendError(req, 400, "invalid_value", "input");
                return;
            }
            BeamerCmd cmd;
            switch (src) {
                case InputSource::HDMI:      cmd = BeamerCmd::INPUT_HDMI;      break;
                case InputSource::VGA:       cmd = BeamerCmd::INPUT_VGA;       break;
                case InputSource::COMPONENT: cmd = BeamerCmd::INPUT_COMPONENT; break;
                case InputSource::SVIDEO:    cmd = BeamerCmd::INPUT_SVIDEO;    break;
                case InputSource::COMPOSITE: cmd = BeamerCmd::INPUT_COMPOSITE; break;
                default: _sendError(req, 400, "invalid_value", "input"); return;
            }
            if (!gRS232.sendCommand(cmd)) { _sendRS232Timeout(req); return; }
            _sendStatus(req);
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
