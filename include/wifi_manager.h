#pragma once

#include <stdbool.h>

enum class WifiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AP_MODE,  // captive portal is active
};

// Initialize Wi-Fi. Loads credentials from NVS and attempts STA connection.
// Falls back to captive portal AP mode if no credentials found or connection
// fails within WIFI_CONNECT_TIMEOUT_S seconds.
void wifi_manager_init();

// Call from main loop. Handles reconnect timer, AP timeout, DNS, etc.
void wifi_manager_update();

// Returns the current Wi-Fi state.
WifiState wifi_manager_get_state();

// Returns true if station is connected and has an IP address.
bool wifi_manager_is_connected();

// Open AP + captive portal. Blocks until credentials are submitted or
// WIFI_PORTAL_TIMEOUT_S elapses.
void wifi_manager_start_portal();

// Clear stored Wi-Fi credentials from NVS and restart in portal mode.
void wifi_manager_clear_credentials();
