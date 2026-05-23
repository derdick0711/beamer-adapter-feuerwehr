#include "ConfigManager.h"

ConfigManager gConfig;

void ConfigManager::load() {
    _prefs.begin("netcfg", true);
    network.ssid     = _prefs.getString("ssid",     "");
    network.password = _prefs.getString("pass",     "");
    network.staticIp = _prefs.getBool("static_ip",  false);
    network.ipAddr   = _prefs.getString("ip_addr",  "");
    network.gateway  = _prefs.getString("ip_gw",    "");
    network.subnet   = _prefs.getString("ip_sub",   "255.255.255.0");
    _prefs.end();

    _prefs.begin("mqttcfg", true);
    mqtt.host   = _prefs.getString("host",   "");
    mqtt.port   = _prefs.getUShort("port",   1883);
    mqtt.prefix = _prefs.getString("prefix", "beamer");
    _prefs.end();

    _prefs.begin("rs232cfg", true);
    rs232.baud  = _prefs.getULong("baud",   9600);
    rs232.rxPin = _prefs.getUChar("rx_pin", 16);
    rs232.txPin = _prefs.getUChar("tx_pin", 17);
    _prefs.end();
}

void ConfigManager::saveNetwork(const NetworkConfig& cfg) {
    network = cfg;
    _prefs.begin("netcfg", false);
    _prefs.putString("ssid",      cfg.ssid);
    _prefs.putString("pass",      cfg.password);
    _prefs.putBool("static_ip",   cfg.staticIp);
    _prefs.putString("ip_addr",   cfg.ipAddr);
    _prefs.putString("ip_gw",     cfg.gateway);
    _prefs.putString("ip_sub",    cfg.subnet);
    _prefs.end();
}

void ConfigManager::saveMqtt(const MqttConfig& cfg) {
    mqtt = cfg;
    _prefs.begin("mqttcfg", false);
    _prefs.putString("host",   cfg.host);
    _prefs.putUShort("port",   cfg.port);
    _prefs.putString("prefix", cfg.prefix);
    _prefs.end();
}

void ConfigManager::saveRs232(const Rs232Config& cfg) {
    rs232 = cfg;
    _prefs.begin("rs232cfg", false);
    _prefs.putULong("baud",   cfg.baud);
    _prefs.putUChar("rx_pin", cfg.rxPin);
    _prefs.putUChar("tx_pin", cfg.txPin);
    _prefs.end();
}

void ConfigManager::loadShelly() {
    _prefs.begin("shellycfg", true);
    shelly.lightIp      = _prefs.getString("light_ip",  "192.168.1.100");
    shelly.screenIp     = _prefs.getString("screen_ip", "192.168.1.101");
    shelly.adminPwHash  = _prefs.getString("admin_pw",  "");
    _prefs.end();
}

void ConfigManager::saveShelly(const ShellyConfig& cfg) {
    shelly = cfg;
    _prefs.begin("shellycfg", false);
    _prefs.putString("light_ip",  cfg.lightIp);
    _prefs.putString("screen_ip", cfg.screenIp);
    _prefs.putString("admin_pw",  cfg.adminPwHash);
    _prefs.end();
}

void ConfigManager::resetWifi() {
    _prefs.begin("netcfg", false);
    _prefs.clear();
    _prefs.end();
    network = NetworkConfig{};
}
