#pragma once
#include <Arduino.h>

struct ShellyConfig {
    String lightIp      = "192.168.1.100";
    String screenIp     = "192.168.1.101";
    String adminPwHash;          // SHA-256 hex; empty = use default "feuerwehr"
    String adminUser    = "admin";
};

struct NetworkConfig {
    String ssid;
    String password;
    bool   staticIp = false;
    String ipAddr;
    String gateway;
    String subnet = "255.255.255.0";
};

struct MqttConfig {
    String   host;
    uint16_t port    = 1883;
    String   prefix  = "beamer";
    bool     enabled = false;
};

struct Rs232Config {
    uint32_t baud  = 9600;
    uint8_t  rxPin = 4;   // D2 on WeMos D1 Mini (GPIO4)
    uint8_t  txPin = 5;   // D1 on WeMos D1 Mini (GPIO5)
};

class ConfigManager {
public:
    NetworkConfig network;
    MqttConfig    mqtt;
    Rs232Config   rs232;
    ShellyConfig  shelly;

    void load();
    void loadShelly();
    void saveNetwork(const NetworkConfig& cfg);
    void saveMqtt(const MqttConfig& cfg);
    void saveRs232(const Rs232Config& cfg);
    void saveShelly(const ShellyConfig& cfg);
    void resetWifi();
    bool hasWifiCredentials() const { return network.ssid.length() > 0; }

private:
    void _save();   // write full config to /config.json
};

extern ConfigManager gConfig;
