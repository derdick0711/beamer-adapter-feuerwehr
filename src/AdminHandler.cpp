#include "AdminHandler.h"
#include "ConfigManager.h"
#include "ShellyClient.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

AdminHandler gAdmin;

// ── SHA-256 ──────────────────────────────────────────────────────────────────

String AdminHandler::hashPassword(const String& plain) {
    uint8_t hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const uint8_t*)plain.c_str(), plain.length());
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    String result;
    result.reserve(64);
    char hex[3];
    for (int i = 0; i < 32; i++) {
        snprintf(hex, sizeof(hex), "%02x", hash[i]);
        result += hex;
    }
    return result;
}

// ── Auth check ───────────────────────────────────────────────────────────────

bool AdminHandler::_checkAuth(AsyncWebServerRequest* req) {
    AsyncWebHeader* h = req->getHeader("Authorization");
    if (!h) return false;

    String auth = h->value();
    if (!auth.startsWith("Basic ")) return false;

    // base64-decode the credentials
    String encoded = auth.substring(6);
    uint8_t buf[128] = {};
    size_t outLen = 0;
    int rc = mbedtls_base64_decode(buf, sizeof(buf) - 1, &outLen,
                                   (const uint8_t*)encoded.c_str(), encoded.length());
    if (rc != 0) return false;

    String decoded((char*)buf, outLen);
    int sep = decoded.indexOf(':');
    if (sep < 0) return false;

    String pass = decoded.substring(sep + 1);

    // Ensure a hash is set (set default on first access)
    if (gConfig.shelly.adminPwHash.length() == 0) {
        gConfig.shelly.adminPwHash = hashPassword("feuerwehr");
        gConfig.saveShelly(gConfig.shelly);
    }

    return hashPassword(pass).equalsIgnoreCase(gConfig.shelly.adminPwHash);
}

// ── Routes ───────────────────────────────────────────────────────────────────

static bool validIp(const String& ip) {
    int dots = 0;
    for (char c : ip) if (c == '.') dots++;
    return dots == 3 && ip.length() >= 7 && ip.length() <= 15;
}

void AdminHandler::begin(AsyncWebServer& server) {
    // GET /admin — serve admin.html (password protected)
    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!gAdmin._checkAuth(req)) {
            req->requestAuthentication("Beamer Adapter Admin");
            return;
        }
        req->send(LittleFS, "/admin.html", "text/html");
    });

    // POST /admin/save — update Shelly IPs and optional password
    server.on("/admin/save", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!gAdmin._checkAuth(req)) {
            req->requestAuthentication("Beamer Adapter Admin");
            return;
        }

        String lightIp  = req->hasParam("light_ip",  true)
                          ? req->getParam("light_ip",  true)->value() : "";
        String screenIp = req->hasParam("screen_ip", true)
                          ? req->getParam("screen_ip", true)->value() : "";
        String newPass  = req->hasParam("new_password", true)
                          ? req->getParam("new_password", true)->value() : "";

        if (!validIp(lightIp) || !validIp(screenIp)) {
            JsonDocument doc;
            doc["error"] = "invalid ip address";
            String body;
            serializeJson(doc, body);
            req->send(400, "application/json", body);
            return;
        }

        gConfig.shelly.lightIp  = lightIp;
        gConfig.shelly.screenIp = screenIp;
        // Update live device IPs immediately (no restart needed)
        gLight.ip  = lightIp;
        gScreen.ip = screenIp;

        if (newPass.length() > 0) {
            gConfig.shelly.adminPwHash = AdminHandler::hashPassword(newPass);
        }

        gConfig.saveShelly(gConfig.shelly);

        req->redirect("/admin");
    });
}
