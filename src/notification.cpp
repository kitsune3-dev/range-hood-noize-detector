#include "notification.h"
#include "config.h"
#include "storage.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static unsigned long s_last_sent_ms = 0;

bool notification_can_send() {
    return (millis() - s_last_sent_ms) >= NOTIF_MIN_INTERVAL_MS;
}

bool notification_send(float db_value, float threshold_db) {
    AppConfig cfg;
    storage_load(cfg);

    if (cfg.ifttt_key.isEmpty()) {
        Serial.println("[notif] IFTTT key not configured — skipping");
        return false;
    }

    // Build URL: https://maker.ifttt.com/trigger/{event}/with/key/{key}
    String url = "https://";
    url += IFTTT_HOST;
    url += "/trigger/";
    url += IFTTT_EVENT_NAME;
    url += "/with/key/";
    url += cfg.ifttt_key;

    // Build JSON body.
    char body[128];
    snprintf(body, sizeof(body),
             "{\"value1\":\"%.1f dB\",\"value2\":\"%.1f dB\",\"value3\":\"range hood\"}",
             db_value, threshold_db);

    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification (acceptable for IFTTT)

    HTTPClient http;
    if (!http.begin(client, url)) {
        Serial.println("[notif] HTTPClient begin failed");
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    http.end();

    if (code == 200) {
        Serial.printf("[notif] IFTTT OK (HTTP %d)\n", code);
        s_last_sent_ms = millis();
        return true;
    }

    Serial.printf("[notif] IFTTT failed (HTTP %d)\n", code);
    return false;
}
