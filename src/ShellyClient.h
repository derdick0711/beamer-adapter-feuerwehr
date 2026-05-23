#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct ShellySwitchDevice {
    String ip;
    String name      = "Deckenlicht";
    bool   output    = false;
    bool   reachable = false;
};

struct ShellyRollerDevice {
    String ip;
    String name       = "Leinwand";
    String state      = "unknown";
    int    currentPos = 0;
    bool   reachable  = false;
};

struct ShellyResult {
    bool   success;
    String error;
};

class ShellyClient {
public:
    static ShellyResult setSwitch(ShellySwitchDevice& dev, bool on);
    static ShellyResult getSwitchStatus(ShellySwitchDevice& dev);

    static ShellyResult coverOpen(ShellyRollerDevice& dev);
    static ShellyResult coverClose(ShellyRollerDevice& dev);
    static ShellyResult coverStop(ShellyRollerDevice& dev);
    static ShellyResult coverGoToPosition(ShellyRollerDevice& dev, int pos);
    static ShellyResult getCoverStatus(ShellyRollerDevice& dev);

private:
    static ShellyResult _get(const String& ip, const String& rpcPath,
                              JsonDocument* out = nullptr);
};

extern ShellySwitchDevice gLight;
extern ShellyRollerDevice gScreen;
