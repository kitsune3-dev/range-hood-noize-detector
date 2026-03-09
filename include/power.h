#pragma once

#include <stdint.h>
#include <stdbool.h>

// Initialize power and Wi-Fi reset button GPIOs.
// Also checks if the Wi-Fi reset button is held at boot.
// Returns true if Wi-Fi credentials should be cleared (reset button held).
bool power_init();

// Call from main loop. Detects long-press on POWER button.
// Returns true if a power-off sequence should begin.
bool power_update();

// Enter ESP32 light sleep for duration_s seconds.
// Wakeup sources: periodic timer + POWER button GPIO.
// After returning, check power_update() to handle button wakeup.
void power_light_sleep(uint32_t duration_s);

// Perform clean shutdown: flush NVS, then enter deep sleep indefinitely.
// Does not return.
void power_shutdown();
