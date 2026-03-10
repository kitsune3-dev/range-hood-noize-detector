#include <Arduino.h>

#include "config.h"
#include "storage.h"
#include "power.h"
#include "audio.h"
#include "wifi_manager.h"
#include "notification.h"

// ── Module-level state ─────────────────────────────────────────────────────

static AppConfig g_config;
static uint8_t   g_consecutive_triggers = 0;

// ── Arduino lifecycle ──────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    Serial.println("[boot] range-hood noise detector starting");

    // 1. Load persisted config (Wi-Fi creds, IFTTT key, threshold).
    storage_load(g_config);
    Serial.printf("[boot] threshold=%.1f dB  IFTTT key=%s\n",
                  g_config.threshold_db,
                  g_config.ifttt_key.isEmpty() ? "(not set)" : "(set)");

    // 2. Power / button init. Returns true if Wi-Fi reset button was held.
    bool reset_wifi = power_init();
    if (reset_wifi) {
        Serial.println("[boot] clearing Wi-Fi credentials (reset button held)");
        storage_clear_wifi();
        storage_load(g_config);
    }

    // 3. Wi-Fi: attempt STA connection or open captive portal.
    wifi_manager_init();

    // 4. I2S microphone.
    if (!audio_init()) {
        Serial.println("[audio] WARNING: I2S init failed — audio measurements disabled");
    }

    Serial.println("[boot] setup complete");
}

void loop() {
    // ── Power button handling ──────────────────────────────────────────────
    if (power_update()) {
        Serial.println("[main] power-off requested");
        audio_deinit();
        power_shutdown();  // does not return
    }

    // ── Wi-Fi maintenance ──────────────────────────────────────────────────
    wifi_manager_update();

    // ── Audio measurement ──────────────────────────────────────────────────
    AudioResult result = audio_measure();

    if (!result.valid) {
        Serial.println("[main] audio measurement invalid — skipping cycle");
    } else {
        // Reload config in case it was updated via captive portal.
        storage_load(g_config);

        Serial.printf("[main] %.1f dB  (threshold=%.1f dB  100Hz=%.2f)\n",
                      result.db_spl, g_config.threshold_db, result.band_energy_100hz);

        if (result.db_spl >= g_config.threshold_db) {
            g_consecutive_triggers++;
            Serial.printf("[main] over-threshold: %d/%d consecutive\n",
                          g_consecutive_triggers, NOISE_SUSTAIN_COUNT);
        } else {
            g_consecutive_triggers = 0;
        }

        if (g_consecutive_triggers > NOISE_SUSTAIN_COUNT) {
            if (wifi_manager_is_connected() && notification_can_send()) {
                bool ok = notification_send(result.db_spl, g_config.threshold_db);
                Serial.printf("[main] notification %s\n", ok ? "sent" : "failed");
                if (ok) {
                    g_consecutive_triggers = 0;
                }
            } else if (!wifi_manager_is_connected()) {
                Serial.println("[main] notification skipped — Wi-Fi not connected");
            }
        }
    }

    // ── Light sleep until next sample interval ─────────────────────────────
    // Skip sleep when in AP/portal mode so the web server stays responsive.
    if (wifi_manager_get_state() != WifiState::AP_MODE) {
        audio_deinit();
        power_light_sleep(NOISE_SAMPLE_INTERVAL_S);
        audio_init();
    }
}
