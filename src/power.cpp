#include "power.h"
#include "config.h"

#include <Arduino.h>
#include "esp_sleep.h"

static unsigned long s_btn_press_start = 0;
static bool          s_btn_was_pressed  = false;

bool power_init() {
    pinMode(PIN_BTN_POWER,    INPUT_PULLUP);
    pinMode(PIN_BTN_WIFI_RST, INPUT_PULLUP);

    // Check if Wi-Fi reset button is held at boot.
    delay(BTN_DEBOUNCE_MS);
    bool wifi_reset_requested = (digitalRead(PIN_BTN_WIFI_RST) == LOW);

    if (wifi_reset_requested) {
        Serial.println("[power] Wi-Fi reset button held at boot");
    }

    return wifi_reset_requested;
}

bool power_update() {
    bool btn_pressed = (digitalRead(PIN_BTN_POWER) == LOW);

    if (btn_pressed && !s_btn_was_pressed) {
        // Button just pressed — start timing.
        s_btn_press_start = millis();
        s_btn_was_pressed = true;
    } else if (!btn_pressed && s_btn_was_pressed) {
        // Button released before long-press threshold.
        s_btn_was_pressed = false;
    } else if (btn_pressed && s_btn_was_pressed) {
        // Button held — check for long press.
        if ((millis() - s_btn_press_start) >= BTN_LONG_PRESS_MS) {
            Serial.println("[power] long press detected → shutdown");
            s_btn_was_pressed = false;
            return true;
        }
    }

    return false;
}

void power_light_sleep(uint32_t duration_s) {
    Serial.printf("[power] light sleep for %u s\n", duration_s);
    Serial.flush();

    esp_sleep_enable_timer_wakeup((uint64_t)duration_s * 1000000ULL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BTN_POWER, 0);  // wake on LOW

    esp_light_sleep_start();

    // Execution resumes here after wakeup.
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[power] woke by button");
    } else {
        Serial.println("[power] woke by timer");
    }
}

void power_shutdown() {
    Serial.println("[power] shutting down — deep sleep indefinitely");
    Serial.flush();

    // Disable all wakeup sources then enter deep sleep.
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
    // Does not return.
}
