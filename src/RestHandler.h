#pragma once
#include <ESPAsyncWebServer.h>
#include "BeamerRS232.h"
#include "BeamerStatus.h"
#include "ConfigManager.h"

class RestHandler {
public:
    void begin(AsyncWebServer& server);

private:
    static void _sendStatus(AsyncWebServerRequest* req);
    static void _sendError(AsyncWebServerRequest* req, int code,
                           const char* error, const char* field = nullptr);
    static void _sendRS232Timeout(AsyncWebServerRequest* req);
};

extern RestHandler gRest;
