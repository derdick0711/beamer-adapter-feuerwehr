#include "WebUI.h"
#include <LittleFS.h>

WebUI gWebUI;

void WebUI::begin(AsyncWebServer& server) {
    // LittleFS already mounted in setup() before this is called

    // Serve static files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Fallback 404
    server.onNotFound([](AsyncWebServerRequest* req) {
        // Let REST paths fall through; only catch non-API 404s
        if (!req->url().startsWith("/api")) {
            req->send(404, "text/plain", "Not found");
        }
    });
}
