#include "AdminHandler.h"
#include "ConfigManager.h"
#include "ShellyClient.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <bearssl/bearssl_hash.h>

AdminHandler gAdmin;

// ── SHA-256 via BearSSL ────────────────────────────────────────────────────────

String AdminHandler::hashPassword(const String& plain) {
    br_sha256_context ctx;
    br_sha256_init(&ctx);
    br_sha256_update(&ctx, plain.c_str(), plain.length());
    uint8_t hash[32];
    br_sha256_out(&ctx, hash);

    String result;
    result.reserve(64);
    char hex[3];
    for (int i = 0; i < 32; i++) {
        snprintf(hex, sizeof(hex), "%02x", hash[i]);
        result += hex;
    }
    return result;
}

// ── Base64 decode (RFC 4648) ───────────────────────────────────────────────────

static int base64Decode(const String& input, uint8_t* output, size_t maxOut) {
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t outLen = 0;
    uint32_t buf = 0;
    int bits = 0;
    for (size_t i = 0; i < input.length() && outLen < maxOut; i++) {
        char c = input.charAt(i);
        if (c == '=') break;
        const char* p = strchr(table, c);
        if (!p) continue;
        buf = (buf << 6) | (uint32_t)(p - table);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output[outLen++] = (buf >> bits) & 0xFF;
        }
    }
    return (int)outLen;
}

// ── Auth check ────────────────────────────────────────────────────────────────

bool AdminHandler::_checkAuth(AsyncWebServerRequest* req) {
    AsyncWebHeader* h = req->getHeader("Authorization");
    if (!h) return false;

    String auth = h->value();
    if (!auth.startsWith("Basic ")) return false;

    String encoded = auth.substring(6);
    uint8_t buf[128] = {};
    int outLen = base64Decode(encoded, buf, sizeof(buf) - 1);
    if (outLen <= 0) return false;

    String decoded((char*)buf, outLen);
    int sep = decoded.indexOf(':');
    if (sep < 0) return false;

    String pass = decoded.substring(sep + 1);

    if (gConfig.shelly.adminPwHash.length() == 0) {
        gConfig.shelly.adminPwHash = hashPassword("feuerwehr");
        gConfig.saveShelly(gConfig.shelly);
    }

    return hashPassword(pass).equalsIgnoreCase(gConfig.shelly.adminPwHash);
}

// ── Routes ────────────────────────────────────────────────────────────────────

static bool validIp(const String& ip) {
    int dots = 0;
    for (char c : ip) if (c == '.') dots++;
    return dots == 3 && ip.length() >= 7 && ip.length() <= 15;
}

void AdminHandler::begin(AsyncWebServer& server) {
    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!gAdmin._checkAuth(req)) {
            req->requestAuthentication("Beamer Adapter Admin");
            return;
        }
        req->send(LittleFS, "/admin.html", "text/html");
    });

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
        gLight.ip  = lightIp;
        gScreen.ip = screenIp;

        if (newPass.length() > 0) {
            gConfig.shelly.adminPwHash = AdminHandler::hashPassword(newPass);
        }

        gConfig.saveShelly(gConfig.shelly);
        req->redirect("/admin");
    });
}
