#include "AdminHandler.h"
#include "ConfigManager.h"
#include "ShellyClient.h"
#include "MqttManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <bearssl/bearssl_hash.h>
#include <ESP8266WiFi.h>

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
    const AsyncWebHeader* h = req->getHeader("Authorization");
    if (!h) return false;

    String auth = h->value();
    if (!auth.startsWith("Basic ")) return false;

    String encoded = auth.substring(6);
    uint8_t buf[128] = {};
    int outLen = base64Decode(encoded, buf, sizeof(buf) - 1);
    if (outLen <= 0) return false;

    String decoded((const char*)buf);
    int sep = decoded.indexOf(':');
    if (sep < 0) return false;

    String user = decoded.substring(0, sep);
    String pass = decoded.substring(sep + 1);

    String expectedUser = gConfig.shelly.adminUser.length() > 0
                          ? gConfig.shelly.adminUser : "admin";
    if (!user.equalsIgnoreCase(expectedUser)) return false;

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
            req->requestAuthentication("Beamer Adapter Admin", false);
            return;
        }
        req->send(LittleFS, "/admin.html", "text/html");
    });

    // GET /admin/config — auth-protected config JSON for admin page
    server.on("/admin/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!gAdmin._checkAuth(req)) {
            req->requestAuthentication("Beamer Adapter Admin", false);
            return;
        }
        JsonDocument doc;
        doc["lightIp"]     = gConfig.shelly.lightIp;
        doc["screenIp"]    = gConfig.shelly.screenIp;
        doc["adminUser"]   = gConfig.shelly.adminUser;
        doc["mqttEnabled"] = gConfig.mqtt.enabled;
        doc["mqttHost"]    = gConfig.mqtt.host;
        doc["mqttPort"]    = gConfig.mqtt.port;
        doc["mqttPrefix"]  = gConfig.mqtt.prefix;
        doc["ssid"]        = gConfig.network.ssid;
        doc["staticIp"]    = gConfig.network.staticIp;
        doc["ipAddr"]      = gConfig.network.ipAddr;
        doc["gateway"]     = gConfig.network.gateway;
        doc["subnet"]      = gConfig.network.subnet;
        doc["currentIp"]   = WiFi.localIP().toString();
        String body;
        serializeJson(doc, body);
        req->send(200, "application/json", body);
    });

    server.on("/admin/save", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!gAdmin._checkAuth(req)) {
            req->requestAuthentication("Beamer Adapter Admin", false);
            return;
        }

        auto p = [&](const char* name) -> String {
            return req->hasParam(name, true) ? req->getParam(name, true)->value() : "";
        };

        String lightIp  = p("light_ip");
        String screenIp = p("screen_ip");
        String newPass  = p("new_password");
        String newUser  = p("new_username");
        bool   mqttEn   = req->hasParam("mqtt_enabled", true);
        String mqttHost = p("mqtt_host");
        String mqttPort = p("mqtt_port");
        String mqttPfx  = p("mqtt_prefix");
        String wifiSsid = p("wifi_ssid");
        String wifiPass = p("wifi_password");
        bool   staticIp = req->hasParam("static_ip", true);
        String ipAddr   = p("ip_addr");
        String ipGw     = p("ip_gw");
        String ipSub    = p("ip_sub");

        // Reject non-empty IPs that fail validation
        if ((lightIp.length()  > 0 && !validIp(lightIp)) ||
            (screenIp.length() > 0 && !validIp(screenIp))) {
            JsonDocument doc;
            doc["error"] = "invalid ip address";
            String body;
            serializeJson(doc, body);
            req->send(400, "application/json", body);
            return;
        }

        // Shelly: partial save
        if (lightIp.length()  > 0) { gConfig.shelly.lightIp  = lightIp;  gLight.ip  = lightIp; }
        if (screenIp.length() > 0) { gConfig.shelly.screenIp = screenIp; gScreen.ip = screenIp; }
        if (newPass.length()  > 0) gConfig.shelly.adminPwHash = AdminHandler::hashPassword(newPass);
        if (newUser.length()  > 0) gConfig.shelly.adminUser   = newUser;
        gConfig.saveShelly(gConfig.shelly);

        // MQTT: detect changes, then partial save
        bool mqttChanged = (mqttEn != gConfig.mqtt.enabled);
        gConfig.mqtt.enabled = mqttEn;
        if (mqttHost.length() > 0 && mqttHost != gConfig.mqtt.host) { mqttChanged = true; gConfig.mqtt.host = mqttHost; }
        if (mqttPort.length() > 0) {
            uint16_t p = (uint16_t)mqttPort.toInt();
            if (p != gConfig.mqtt.port) { mqttChanged = true; gConfig.mqtt.port = p; }
        }
        if (mqttPfx.length() > 0 && mqttPfx != gConfig.mqtt.prefix) { mqttChanged = true; gConfig.mqtt.prefix = mqttPfx; }
        gConfig.saveMqtt(gConfig.mqtt);

        // WiFi: partial save
        bool wifiChanged = false;
        if (wifiSsid.length() > 0) { wifiChanged = true; gConfig.network.ssid     = wifiSsid; }
        if (wifiPass.length() > 0) { wifiChanged = true; gConfig.network.password = wifiPass; }
        if (staticIp != gConfig.network.staticIp) { wifiChanged = true; gConfig.network.staticIp = staticIp; }
        if (ipAddr.length() > 0) { wifiChanged = true; gConfig.network.ipAddr   = ipAddr; }
        if (ipGw.length()   > 0) { wifiChanged = true; gConfig.network.gateway  = ipGw; }
        if (ipSub.length()  > 0) { wifiChanged = true; gConfig.network.subnet   = ipSub; }
        if (wifiChanged) gConfig.saveNetwork(gConfig.network);

        // Smart redirect: append restart flags so browser shows the banner
        if      (wifiChanged && mqttChanged) req->redirect("/admin?wifi=1&mqtt=1");
        else if (wifiChanged)                req->redirect("/admin?wifi=1");
        else if (mqttChanged)                req->redirect("/admin?mqtt=1");
        else                                 req->redirect("/admin");
    });

    // POST /admin/restart
    server.on("/admin/restart", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!gAdmin._checkAuth(req)) {
            req->requestAuthentication("Beamer Adapter Admin", false);
            return;
        }
        req->send(200, "text/plain", "restarting");
        delay(200);
        ESP.restart();
    });
}
