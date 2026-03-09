#pragma once

#include <Arduino.h>

struct AppConfig {
    String wifi_ssid;
    String wifi_pass;
    String ifttt_key;
    float  threshold_db;  // noise detection threshold in dB SPL
};

// Load config from NVS into dst. Missing keys receive default values.
// Returns true on success.
bool storage_load(AppConfig &dst);

// Persist config to NVS.
// Returns true on success.
bool storage_save(const AppConfig &src);

// Erase only the Wi-Fi credentials (for Wi-Fi reset button workflow).
// Returns true on success.
bool storage_clear_wifi();
