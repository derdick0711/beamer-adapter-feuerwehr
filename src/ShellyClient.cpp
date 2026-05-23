#include "ShellyClient.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

ShellySwitchDevice gLight;
ShellyRollerDevice gScreen;

ShellyResult ShellyClient::_get(const String& ip, const String& rpcPath,
                                 JsonDocument* out) {
    if (ip.length() == 0) return {false, "no_ip"};

    WiFiClient wifiClient;
    HTTPClient http;
    String url = "http://" + ip + "/rpc/" + rpcPath;
    http.begin(wifiClient, url);
    http.setTimeout(2500);
    int code = http.GET();

    if (code != 200) {
        http.end();
        return {false, "http_error_" + String(code < 0 ? -code : code)};
    }

    if (out) {
        String body = http.getString();
        deserializeJson(*out, body);
    }
    http.end();
    return {true, ""};
}

ShellyResult ShellyClient::setSwitch(ShellySwitchDevice& dev, bool on) {
    String rpc = String("Switch.Set?id=0&on=") + (on ? "true" : "false");
    ShellyResult r = _get(dev.ip, rpc);
    dev.reachable = r.success;
    if (r.success) dev.output = on;
    return r;
}

ShellyResult ShellyClient::getSwitchStatus(ShellySwitchDevice& dev) {
    JsonDocument doc;
    ShellyResult r = _get(dev.ip, "Switch.GetStatus?id=0", &doc);
    dev.reachable = r.success;
    if (r.success && doc["output"].is<bool>()) {
        dev.output = doc["output"].as<bool>();
    }
    return r;
}

ShellyResult ShellyClient::coverOpen(ShellyRollerDevice& dev) {
    ShellyResult r = _get(dev.ip, "Cover.Open?id=0");
    dev.reachable = r.success;
    if (r.success) dev.state = "opening";
    return r;
}

ShellyResult ShellyClient::coverClose(ShellyRollerDevice& dev) {
    ShellyResult r = _get(dev.ip, "Cover.Close?id=0");
    dev.reachable = r.success;
    if (r.success) dev.state = "closing";
    return r;
}

ShellyResult ShellyClient::coverStop(ShellyRollerDevice& dev) {
    ShellyResult r = _get(dev.ip, "Cover.Stop?id=0");
    dev.reachable = r.success;
    if (r.success) dev.state = "stopped";
    return r;
}

ShellyResult ShellyClient::coverGoToPosition(ShellyRollerDevice& dev, int pos) {
    String rpc = "Cover.GoToPosition?id=0&pos=" + String(pos);
    ShellyResult r = _get(dev.ip, rpc);
    dev.reachable = r.success;
    if (r.success) dev.state = "moving";
    return r;
}

ShellyResult ShellyClient::getCoverStatus(ShellyRollerDevice& dev) {
    JsonDocument doc;
    ShellyResult r = _get(dev.ip, "Cover.GetStatus?id=0", &doc);
    dev.reachable = r.success;
    if (r.success) {
        if (doc["state"].is<const char*>())
            dev.state = doc["state"].as<String>();
        if (doc["current_pos"].is<int>())
            dev.currentPos = doc["current_pos"].as<int>();
    }
    return r;
}
