#include "MqttManager.h"
#include <ArduinoJson.h>

MqttManager  gMqtt;
MqttManager* MqttManager::_instance = nullptr;

void MqttManager::begin(ConfigManager& cfg) {
    if (!cfg.mqtt.enabled || cfg.mqtt.host.length() == 0) return;

    _instance = this;
    _prefix   = cfg.mqtt.prefix;

    _client.setClient(_wifiClient);
    _client.setServer(cfg.mqtt.host.c_str(), cfg.mqtt.port);
    _client.setCallback(_onMessage);
    _client.setBufferSize(512);

    _reconnect();
    publishStatus();
}

void MqttManager::loop() {
    if (_client.connected()) {
        _client.loop();
        return;
    }
    // Exponential back-off: retry no faster than 5 s, max 60 s
    uint32_t now = millis();
    static uint32_t backoff = 5000;
    if (now - _lastReconnectAttempt > backoff) {
        _lastReconnectAttempt = now;
        _reconnect();
        backoff = min(backoff * 2, (uint32_t)60000);
    }
}

bool MqttManager::isConnected() {
    return _client.connected();
}

void MqttManager::publishStatus() {
    if (!_client.connected()) return;

    JsonDocument doc;
    doc["power"]     = (gBeamerStatus.power == PowerState::ON)  ? "on" :
                       (gBeamerStatus.power == PowerState::OFF) ? "off" : "unknown";
    doc["input"]     = BeamerRS232::inputName(gBeamerStatus.input);
    doc["blank"]     = gBeamerStatus.blank;
    doc["reachable"] = gBeamerStatus.reachable;
    doc["uptime"]    = millis() / 1000;

    char buf[256];
    size_t n = serializeJson(doc, buf, sizeof(buf));

    String statTopic = _prefix + "/stat";
    _client.publish(statTopic.c_str(), (uint8_t*)buf, n, true /*retain*/);
}

void MqttManager::_reconnect() {
    if (!_instance) return;

    String clientId = "beamer-adapter-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macSuffix[7];
    snprintf(macSuffix, sizeof(macSuffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    clientId += macSuffix;

    String lwtTopic = _prefix + "/stat";
    const char* lwtPayload = "{\"reachable\":false,\"reason\":\"lwt\"}";

    if (_client.connect(clientId.c_str(),
                        nullptr, nullptr,          // no auth
                        lwtTopic.c_str(), 0, true, // LWT: QoS0, retain
                        lwtPayload)) {
        _subscribe();
        static uint32_t backoff = 5000; backoff = 5000; // reset on success
        String ipTopic = _prefix + "/ip";
        String ip = WiFi.localIP().toString();
        _client.publish(ipTopic.c_str(), ip.c_str(), true /*retain*/);
    }
}

void MqttManager::publishLightStatus() {
    if (!_client.connected()) return;
    JsonDocument doc;
    doc["output"]    = gLight.output;
    doc["reachable"] = gLight.reachable;
    char buf[64];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    String topic = _prefix + "/stat/light";
    _client.publish(topic.c_str(), (uint8_t*)buf, n, true /*retain*/);
}

void MqttManager::publishScreenStatus() {
    if (!_client.connected()) return;
    JsonDocument doc;
    doc["state"]       = gScreen.state;
    doc["current_pos"] = gScreen.currentPos;
    doc["reachable"]   = gScreen.reachable;
    char buf[96];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    String topic = _prefix + "/stat/screen";
    _client.publish(topic.c_str(), (uint8_t*)buf, n, true /*retain*/);
}

void MqttManager::publishSceneResult(const SceneResult& r) {
    if (!_client.connected()) return;
    String body = r.toJson();
    String topic = _prefix + "/stat/scene";
    _client.publish(topic.c_str(), (uint8_t*)body.c_str(), body.length(), false);
}

void MqttManager::_subscribe() {
    _client.subscribe((_prefix + "/cmnd/power").c_str());
    _client.subscribe((_prefix + "/cmnd/input").c_str());
    _client.subscribe((_prefix + "/cmnd/blank").c_str());
    _client.subscribe((_prefix + "/cmnd/light").c_str());
    _client.subscribe((_prefix + "/cmnd/screen").c_str());
    _client.subscribe((_prefix + "/cmnd/scene/start").c_str());
    _client.subscribe((_prefix + "/cmnd/scene/stop").c_str());
}

void MqttManager::_onMessage(char* topic, uint8_t* payload, unsigned int len) {
    if (!_instance) return;

    String topicStr(topic);
    String msg;
    for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
    msg.trim();

    String prefix = _instance->_prefix;

    if (topicStr == prefix + "/cmnd/power") {
        BeamerCmd cmd;
        if (msg == "on")       cmd = BeamerCmd::POWER_ON;
        else if (msg == "off") cmd = BeamerCmd::POWER_OFF;
        else return;
        gRS232.sendCommand(cmd);
        _instance->publishStatus();

    } else if (topicStr == prefix + "/cmnd/input") {
        InputSource src = BeamerRS232::inputFromString(msg);
        if (src != InputSource::HDMI && src != InputSource::VGA) return;
        BeamerCmd cmd = (src == InputSource::HDMI) ? BeamerCmd::INPUT_HDMI : BeamerCmd::INPUT_VGA;
        gRS232.sendCommand(cmd);
        _instance->publishStatus();

    } else if (topicStr == prefix + "/cmnd/blank") {
        BeamerCmd cmd;
        if (msg == "true")       cmd = BeamerCmd::BLANK_ON;
        else if (msg == "false") cmd = BeamerCmd::BLANK_OFF;
        else return;
        gRS232.sendCommand(cmd);
        _instance->publishStatus();

    } else if (topicStr == prefix + "/cmnd/light") {
        if (msg != "on" && msg != "off") return;
        ShellyClient::setSwitch(gLight, msg == "on");
        _instance->publishLightStatus();

    } else if (topicStr == prefix + "/cmnd/screen") {
        if      (msg == "open")  ShellyClient::coverOpen(gScreen);
        else if (msg == "close") ShellyClient::coverClose(gScreen);
        else if (msg == "stop")  ShellyClient::coverStop(gScreen);
        else if (msg.startsWith("pos:")) {
            int pos = msg.substring(4).toInt();
            if (pos >= 0 && pos <= 100)
                ShellyClient::coverGoToPosition(gScreen, pos);
            else return;
        } else return;
        _instance->publishScreenStatus();

    } else if (topicStr == prefix + "/cmnd/scene/start") {
        SceneResult r = SceneManager::runStart();
        _instance->publishSceneResult(r);
        _instance->publishLightStatus();
        _instance->publishScreenStatus();

    } else if (topicStr == prefix + "/cmnd/scene/stop") {
        SceneResult r = SceneManager::runStop();
        _instance->publishSceneResult(r);
        _instance->publishLightStatus();
        _instance->publishScreenStatus();
    }
}
