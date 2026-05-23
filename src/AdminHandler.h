#pragma once
#include <ESPAsyncWebServer.h>
#include <Arduino.h>

class AdminHandler {
public:
    void begin(AsyncWebServer& server);

    // Returns SHA-256 hex string of plain-text password
    static String hashPassword(const String& plain);

    bool _checkAuth(AsyncWebServerRequest* req);
};

extern AdminHandler gAdmin;
