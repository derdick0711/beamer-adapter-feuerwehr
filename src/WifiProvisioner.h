#pragma once
#include <Arduino.h>
#include "ConfigManager.h"

class WifiProvisioner {
public:
    // Checks boot-time reset button, then connects or opens AP
    void begin(ConfigManager& cfg);

    // Clears WiFi credentials and restarts into AP mode
    void resetConfig(ConfigManager& cfg);

private:
    static constexpr uint8_t  RESET_PIN    = 0;
    static constexpr uint32_t AP_TIMEOUT_S = 300;
};

extern WifiProvisioner gWifi;
