#pragma once
#include <Arduino.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.0.0-dev"
#endif

class OtaUpdater {
public:
    void begin(const char* owner, const char* repo);
    void loop();

    static const char* currentVersion() { return FIRMWARE_VERSION; }

private:
    const char* _owner = nullptr;
    const char* _repo  = nullptr;
    uint32_t    _lastCheck = 0;

    // Check once per hour; first check happens after one full interval
    static constexpr uint32_t CHECK_INTERVAL_MS = 60UL * 60UL * 1000UL;

    void   _checkAndUpdate();
    bool   _newerVersion(const String& latestTag) const;
};

extern OtaUpdater gOta;
