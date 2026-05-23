#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

#include "ConfigManager.h"
#include "BeamerStatus.h"
#include "BeamerRS232.h"
#include "WifiProvisioner.h"
#include "RestHandler.h"
#include "MqttManager.h"
#include "WebUI.h"
#include "ShellyClient.h"
#include "AdminHandler.h"

BeamerStatus gBeamerStatus;

static AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[boot] Beamer Adapter Feuerwehr starting...");

    // 1. Mount filesystem (required before config load and web UI)
    if (!LittleFS.begin()) {
        Serial.println("[boot] LittleFS mount failed — formatting...");
        LittleFS.format();
        LittleFS.begin();
    }

    // 2. Load config from LittleFS
    gConfig.load();
    gConfig.loadShelly();

    // Init Shelly device IPs from config
    gLight.ip  = gConfig.shelly.lightIp;
    gScreen.ip = gConfig.shelly.screenIp;

    // 3. WiFi provisioning (blocks until connected or AP timeout → restart)
    gWifi.begin(gConfig);

    Serial.print("[boot] IP: ");
    Serial.println(WiFi.localIP());

    // 4. RS232 SoftwareSerial
    gRS232.begin(gConfig.rs232);

    // 5. Web UI (LittleFS) + REST API + Admin UI
    gWebUI.begin(server);
    gRest.begin(server);
    gAdmin.begin(server);
    server.begin();
    Serial.println("[boot] HTTP server started");

    // 6. MQTT (non-blocking; skipped if no broker configured)
    gMqtt.begin(gConfig);

    Serial.println("[boot] Ready");
}

void loop() {
    // MQTT keepalive + auto-reconnect
    gMqtt.loop();

    // Reset-button 5 s hold detection
    gWifi.checkResetButton(gConfig);

    delay(10);
}
