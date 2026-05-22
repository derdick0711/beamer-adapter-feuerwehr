#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>

#include "ConfigManager.h"
#include "BeamerStatus.h"
#include "BeamerRS232.h"
#include "WifiProvisioner.h"
#include "RestHandler.h"
#include "MqttManager.h"
#include "WebUI.h"

BeamerStatus gBeamerStatus;

static AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[boot] Beamer Adapter Feuerwehr starting...");

    // Watchdog: 10 s timeout (constitution §III)
    esp_task_wdt_config_t wdt_cfg = { .timeout_ms = 10000, .idle_core_mask = 0, .trigger_panic = true };
    esp_task_wdt_reconfigure(&wdt_cfg);
    esp_task_wdt_add(nullptr);

    // 1. Load config from NVS
    gConfig.load();

    // 2. WiFi provisioning (blocks until connected or AP timeout → restart)
    gWifi.begin(gConfig);

    Serial.print("[boot] IP: ");
    Serial.println(WiFi.localIP());

    // 3. RS232 UART
    gRS232.begin(gConfig.rs232);

    // 4. Web UI (LittleFS) + REST API
    gWebUI.begin(server);
    gRest.begin(server);
    server.begin();
    Serial.println("[boot] HTTP server started");

    // 5. MQTT (non-blocking; skipped if no broker configured)
    gMqtt.begin(gConfig);

    Serial.println("[boot] Ready");
    esp_task_wdt_reset();
}

void loop() {
    esp_task_wdt_reset();

    // MQTT keepalive + auto-reconnect
    gMqtt.loop();

    // Reset-button 5 s hold detection
    gWifi.checkResetButton(gConfig);

    delay(10);
}
