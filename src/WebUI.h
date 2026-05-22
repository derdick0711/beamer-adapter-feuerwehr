#pragma once
#include <ESPAsyncWebServer.h>

class WebUI {
public:
    // Mount LittleFS and register GET / route
    void begin(AsyncWebServer& server);
};

extern WebUI gWebUI;
