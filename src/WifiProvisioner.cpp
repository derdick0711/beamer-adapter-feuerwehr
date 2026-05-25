#include "WifiProvisioner.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

WifiProvisioner gWifi;

void WifiProvisioner::begin(ConfigManager& cfg) {
    pinMode(RESET_PIN, INPUT_PULLUP);

    // Boot-time reset: button held at startup → force AP mode
    if (digitalRead(RESET_PIN) == LOW) {
        delay(2000);
        if (digitalRead(RESET_PIN) == LOW) {
            cfg.resetWifi();
        }
    }

    WiFiManager wm;
    wm.setConfigPortalTimeout(AP_TIMEOUT_S);

    // Custom params: Static IP
    WiFiManagerParameter paramStaticIp("static_ip", "Statische IP (leer = DHCP)", cfg.network.ipAddr.c_str(), 16);
    WiFiManagerParameter paramGw("ip_gw", "Gateway", cfg.network.gateway.c_str(), 16);
    WiFiManagerParameter paramSubnet("ip_sub", "Subnetzmaske", cfg.network.subnet.c_str(), 16);

    // Custom params: MQTT
    WiFiManagerParameter paramMqttHost("mqtt_host", "MQTT Broker IP/Host", cfg.mqtt.host.c_str(), 64);
    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", cfg.mqtt.port);
    WiFiManagerParameter paramMqttPort("mqtt_port", "MQTT Port", portStr, 8);
    WiFiManagerParameter paramMqttPrefix("mqtt_prefix", "MQTT Prefix", cfg.mqtt.prefix.c_str(), 32);

    wm.addParameter(&paramStaticIp);
    wm.addParameter(&paramGw);
    wm.addParameter(&paramSubnet);
    wm.addParameter(&paramMqttHost);
    wm.addParameter(&paramMqttPort);
    wm.addParameter(&paramMqttPrefix);

    // Save callback
    wm.setSaveParamsCallback([&]() {
        NetworkConfig netCfg;
        netCfg.ssid     = WiFi.SSID();
        netCfg.password = WiFi.psk();
        String ip = String(paramStaticIp.getValue());
        if (ip.length() > 0) {
            netCfg.staticIp = true;
            netCfg.ipAddr   = ip;
            netCfg.gateway  = String(paramGw.getValue());
            netCfg.subnet   = String(paramSubnet.getValue());
        }
        cfg.saveNetwork(netCfg);

        MqttConfig mqttCfg;
        mqttCfg.host   = String(paramMqttHost.getValue());
        mqttCfg.port   = (uint16_t)atoi(paramMqttPort.getValue());
        mqttCfg.prefix = String(paramMqttPrefix.getValue());
        if (mqttCfg.prefix.length() == 0) mqttCfg.prefix = "beamer";
        if (mqttCfg.port == 0)            mqttCfg.port   = 1883;
        cfg.saveMqtt(mqttCfg);
    });

    // Apply static IP before connecting if configured
    if (cfg.network.staticIp && cfg.network.ipAddr.length() > 0) {
        IPAddress ip, gw, sn;
        if (ip.fromString(cfg.network.ipAddr) &&
            gw.fromString(cfg.network.gateway) &&
            sn.fromString(cfg.network.subnet)) {
            WiFi.config(ip, gw, sn);
        }
    }

    bool connected;
    if (!cfg.hasWifiCredentials()) {
        connected = wm.startConfigPortal("BeamerAdapter-Setup");
    } else {
        connected = wm.autoConnect("BeamerAdapter-Setup");
    }
    if (!connected) {
        ESP.restart();
    }
}

void WifiProvisioner::resetConfig(ConfigManager& cfg) {
    cfg.resetWifi();
    delay(200);
    ESP.restart();
}
