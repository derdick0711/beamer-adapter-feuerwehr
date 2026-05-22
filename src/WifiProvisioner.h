#pragma once
#include <Arduino.h>
#include "ConfigManager.h"

class WifiProvisioner {
public:
    // Blocks until connected or AP config saved. Sets up custom params.
    void begin(ConfigManager& cfg);

    // Clears WiFi NVS keys and restarts into AP mode
    void resetConfig(ConfigManager& cfg);

    // Call in loop() to detect 5s reset-button hold (GPIO0)
    void checkResetButton(ConfigManager& cfg);

private:
    static constexpr uint8_t  RESET_PIN    = 0;   // BOOT button on DevKit
    static constexpr uint32_t HOLD_MS      = 5000;
    static constexpr uint32_t AP_TIMEOUT_S = 300; // 5 minutes

    uint32_t _btnPressStart = 0;
    bool     _btnHeld       = false;
};

extern WifiProvisioner gWifi;
