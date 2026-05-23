#pragma once
#include <Arduino.h>

struct SceneStep {
    String device;   // "screen", "light", "beamer"
    String action;   // "close", "open", "off", "on", "power_on", "power_off", "hdmi"
    bool   success;
    String error;
};

struct SceneResult {
    String    scene;       // "start" or "stop"
    bool      success;
    uint32_t  duration_ms;
    SceneStep steps[4];
    uint8_t   stepCount = 0;

    String toJson() const;
};

class SceneManager {
public:
    static SceneResult runStart();  // screen close → light off → beamer on → hdmi
    static SceneResult runStop();   // screen open  → light on  → beamer off
};
