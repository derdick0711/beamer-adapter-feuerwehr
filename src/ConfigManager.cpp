#include "ConfigManager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

ConfigManager gConfig;

static const char* CFG_FILE = "/config.json";

void ConfigManager::load() {
    File f = LittleFS.open(CFG_FILE, "r");
    if (!f) return;  // first boot — keep defaults

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return; }
    f.close();

    // Network
    network.ssid      = doc["ssid"]      | "";
    network.password  = doc["pass"]      | "";
    network.staticIp  = doc["static_ip"] | false;
    network.ipAddr    = doc["ip_addr"]   | "";
    network.gateway   = doc["ip_gw"]     | "";
    network.subnet    = doc["ip_sub"]    | "255.255.255.0";

    // MQTT
    mqtt.host    = doc["mqtt_host"]     | "";
    mqtt.port    = doc["mqtt_port"]     | (uint16_t)1883;
    mqtt.prefix  = doc["mqtt_prefix"]   | "beamer";
    mqtt.enabled = doc["mqtt_enabled"]  | false;

    // RS232
    rs232.baud  = doc["rs232_baud"]  | (uint32_t)9600;
    rs232.rxPin = doc["rs232_rx"]    | (uint8_t)4;
    rs232.txPin = doc["rs232_tx"]    | (uint8_t)5;
}

void ConfigManager::loadShelly() {
    File f = LittleFS.open(CFG_FILE, "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return; }
    f.close();

    shelly.lightIp     = doc["light_ip"]   | "192.168.1.100";
    shelly.screenIp    = doc["screen_ip"]  | "192.168.1.101";
    shelly.adminPwHash = doc["admin_pw"]   | "";
    shelly.adminUser   = doc["admin_user"] | "admin";
}

void ConfigManager::_save() {
    JsonDocument doc;
    doc["ssid"]       = network.ssid;
    doc["pass"]       = network.password;
    doc["static_ip"]  = network.staticIp;
    doc["ip_addr"]    = network.ipAddr;
    doc["ip_gw"]      = network.gateway;
    doc["ip_sub"]     = network.subnet;
    doc["mqtt_host"]    = mqtt.host;
    doc["mqtt_port"]    = mqtt.port;
    doc["mqtt_prefix"]  = mqtt.prefix;
    doc["mqtt_enabled"] = mqtt.enabled;
    doc["rs232_baud"] = rs232.baud;
    doc["rs232_rx"]   = rs232.rxPin;
    doc["rs232_tx"]   = rs232.txPin;
    doc["light_ip"]   = shelly.lightIp;
    doc["screen_ip"]  = shelly.screenIp;
    doc["admin_pw"]   = shelly.adminPwHash;
    doc["admin_user"] = shelly.adminUser;

    File f = LittleFS.open(CFG_FILE, "w");
    if (f) { serializeJson(doc, f); f.close(); }
}

void ConfigManager::saveNetwork(const NetworkConfig& cfg) {
    network = cfg;
    _save();
}

void ConfigManager::saveMqtt(const MqttConfig& cfg) {
    mqtt = cfg;
    _save();
}

void ConfigManager::saveRs232(const Rs232Config& cfg) {
    rs232 = cfg;
    _save();
}

void ConfigManager::saveShelly(const ShellyConfig& cfg) {
    shelly = cfg;
    _save();
}

void ConfigManager::resetWifi() {
    network = NetworkConfig{};
    _save();
}
