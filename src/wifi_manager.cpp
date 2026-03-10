#include "wifi_manager.h"
#include "config.h"
#include "storage.h"

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ── Module state ───────────────────────────────────────────────────────────

static WifiState      s_state          = WifiState::DISCONNECTED;
static unsigned long  s_last_reconnect = 0;
static DNSServer      s_dns;
static AsyncWebServer s_server(80);
static bool           s_server_running = false;
static unsigned long  s_portal_start   = 0;

// ── Internal helpers ───────────────────────────────────────────────────────

static void start_ap_mode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD[0] ? WIFI_AP_PASSWORD : nullptr);
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );

    // DNS: redirect every query to the captive portal IP.
    s_dns.start(53, "*", IPAddress(192, 168, 4, 1));

    Serial.printf("[wifi] AP started: SSID=%s  IP=%s\n",
                  WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
}

static void setup_portal_routes() {
    if (!LittleFS.begin(true)) {
        Serial.println("[wifi] LittleFS mount failed");
    }

    // Serve static files from LittleFS (captive portal UI).
    s_server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Wi-Fi scan endpoint.
    s_server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanNetworks();
        JsonDocument doc;
        JsonArray arr = doc["networks"].to<JsonArray>();
        for (int i = 0; i < n; i++) {
            arr.add(WiFi.SSID(i));
        }
        String body;
        serializeJson(doc, body);
        request->send(200, "application/json", body);
    });

    // Save credentials endpoint.
    s_server.on("/api/save", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Parameters are parsed in the body handler below.
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "application/json", "{\"error\":\"invalid json\"}");
                return;
            }

            AppConfig cfg;
            storage_load(cfg);
            cfg.wifi_ssid    = doc["ssid"].as<String>();
            cfg.wifi_pass    = doc["pass"].as<String>();
            if (doc.containsKey("ifttt_key")) {
                cfg.ifttt_key = doc["ifttt_key"].as<String>();
            }
            if (doc.containsKey("threshold")) {
                cfg.threshold_db = doc["threshold"].as<float>();
            }
            storage_save(cfg);

            request->send(200, "application/json", "{\"status\":\"ok\"}");

            // Restart into STA mode after a short delay.
            delay(500);
            ESP.restart();
        }
    );

    // Device status endpoint — returns current config for diagnostics.
    // NOTE: No authentication. This route MUST only be registered in AP mode
    // (called exclusively from start_ap_portal()) to avoid leaking config on
    // a shared LAN in STA mode.
    s_server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        AppConfig cfg;
        storage_load(cfg);
        JsonDocument doc;
        doc["ssid"]            = cfg.wifi_ssid;
        doc["threshold_db"]    = cfg.threshold_db;
        doc["ifttt_key_set"]   = (cfg.ifttt_key[0] != '\0');  // presence only, not the key itself
        String body;
        serializeJson(doc, body);
        request->send(200, "application/json", body);
    });

    // Captive portal redirect for iOS/Android detection probes.
    s_server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("http://" WIFI_AP_IP "/");
    });

    s_server.begin();
    s_server_running = true;
    Serial.println("[wifi] captive portal web server started");
}

static bool try_connect(const AppConfig &cfg) {
    if (cfg.wifi_ssid.isEmpty()) {
        return false;
    }

    Serial.printf("[wifi] connecting to SSID: %s\n", cfg.wifi_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > (unsigned long)WIFI_CONNECT_TIMEOUT_S * 1000UL) {
            Serial.println("[wifi] connection timed out");
            return false;
        }
        delay(200);
    }

    Serial.printf("[wifi] connected, IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

// ── Public API ─────────────────────────────────────────────────────────────

void wifi_manager_init() {
    AppConfig cfg;
    storage_load(cfg);

    if (try_connect(cfg)) {
        s_state = WifiState::CONNECTED;
    } else {
        wifi_manager_start_portal();
    }
}

void wifi_manager_update() {
    if (s_state == WifiState::AP_MODE) {
        s_dns.processNextRequest();

        // Portal timeout.
        if ((millis() - s_portal_start) > (unsigned long)WIFI_PORTAL_TIMEOUT_S * 1000UL) {
            Serial.println("[wifi] portal timed out");
            s_state = WifiState::DISCONNECTED;
        }
        return;
    }

    if (s_state == WifiState::CONNECTED && WiFi.status() != WL_CONNECTED) {
        Serial.println("[wifi] connection lost");
        s_state = WifiState::DISCONNECTED;
    }

    if (s_state == WifiState::DISCONNECTED) {
        if (millis() - s_last_reconnect > (unsigned long)WIFI_RECONNECT_INTERVAL_S * 1000UL) {
            s_last_reconnect = millis();
            s_state          = WifiState::CONNECTING;

            AppConfig cfg;
            storage_load(cfg);
            if (try_connect(cfg)) {
                s_state = WifiState::CONNECTED;
            } else {
                s_state = WifiState::DISCONNECTED;
            }
        }
    }
}

WifiState wifi_manager_get_state() {
    return s_state;
}

bool wifi_manager_is_connected() {
    return s_state == WifiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}

void wifi_manager_start_portal() {
    Serial.println("[wifi] starting captive portal");
    start_ap_mode();
    setup_portal_routes();
    s_portal_start = millis();
    s_state        = WifiState::AP_MODE;
}

void wifi_manager_clear_credentials() {
    storage_clear_wifi();
    wifi_manager_start_portal();
}
