#pragma once
#include <Arduino.h>

enum class PowerState  { ON, OFF, UNKNOWN };
enum class InputSource { HDMI, VGA, COMPONENT, SVIDEO, COMPOSITE, UNKNOWN };

struct BeamerStatus {
    PowerState  power      = PowerState::UNKNOWN;
    InputSource input      = InputSource::UNKNOWN;
    bool        blank      = false;
    bool        reachable  = false;
    uint32_t    lastUpdated = 0;

    void markReachable()   { reachable = true;  lastUpdated = millis(); }
    void markUnreachable() { reachable = false; lastUpdated = millis(); }
};

// Global singleton — updated by BeamerRS232 after each command
extern BeamerStatus gBeamerStatus;
