#pragma once
#include <Arduino.h>
#include "BeamerStatus.h"
#include "ConfigManager.h"

enum class BeamerCmd {
    POWER_ON, POWER_OFF,
    INPUT_HDMI, INPUT_VGA, INPUT_COMPONENT, INPUT_SVIDEO, INPUT_COMPOSITE,
    BLANK_ON, BLANK_OFF
};

class BeamerRS232 {
public:
    void begin(const Rs232Config& cfg);

    // Returns true on ACK, false on timeout/NAK (updates gBeamerStatus)
    bool sendCommand(BeamerCmd cmd);

    static const char* inputName(InputSource src);
    static InputSource inputFromString(const String& s);

private:
    HardwareSerial* _serial = nullptr;
    bool _waitAck(uint32_t timeoutMs = 500);
    void _flushRx();
    static const char* _cmdStr(BeamerCmd cmd);
};

extern BeamerRS232 gRS232;
