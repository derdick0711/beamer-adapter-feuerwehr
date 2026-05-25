#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "ConfigManager.h"
#include "BeamerRS232.h"
#include "BeamerStatus.h"
#include "ShellyClient.h"
#include "SceneManager.h"

class MqttManager {
public:
    void begin(ConfigManager& cfg);
    void loop();
    bool isConnected();
    void publishStatus();           // beamer status after RS232 command
    void publishLightStatus();      // beamer/stat/light
    void publishScreenStatus();     // beamer/stat/screen
    void publishSceneResult(const SceneResult& r);  // beamer/stat/scene

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
