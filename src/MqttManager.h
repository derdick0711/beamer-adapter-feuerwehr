#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "ConfigManager.h"
#include "BeamerRS232.h"
#include "BeamerStatus.h"

class MqttManager {
public:
    void begin(ConfigManager& cfg);
    void loop();
    void publishStatus();   // Call after every command execution

private:
    WiFiClient    _wifiClient;
    PubSubClient  _client;
    String        _prefix;
    uint32_t      _lastReconnectAttempt = 0;

    void _reconnect();
    void _subscribe();
    static void _onMessage(char* topic, uint8_t* payload, unsigned int len);

    static MqttManager* _instance; // for static callback
};

extern MqttManager gMqtt;
