#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct ShellyConfig {
    String lightIp      = "192.168.1.100";
    String screenIp     = "192.168.1.101";
    String adminPwHash;  // SHA-256 hex; empty = use default "feuerwehr"
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
    String  host;
    uint16_t port   = 1883;
    String  prefix  = "beamer";
};

struct Rs232Config {
    uint32_t baud  = 9600;
    uint8_t  rxPin = 16;
    uint8_t  txPin = 17;
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
    Preferences _prefs;
};

extern ConfigManager gConfig;
