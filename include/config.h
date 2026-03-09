#pragma once

// ── Pin assignments ────────────────────────────────────────────────────────
#define PIN_I2S_SCK       26   // INMP441 serial clock (SCK)
#define PIN_I2S_WS        25   // INMP441 word select / L-R clock (WS)
#define PIN_I2S_SD        34   // INMP441 serial data (SD) — input-only pin

#define PIN_BTN_POWER     0    // Long-press power on/off (reuses BOOT button)
#define PIN_BTN_WIFI_RST  35   // Held at boot → clears NVS Wi-Fi credentials

// ── I2S / Audio ───────────────────────────────────────────────────────────
#define AUDIO_SAMPLE_RATE        44100  // Hz
#define AUDIO_SAMPLE_BITS        32     // INMP441 outputs 24-bit in a 32-bit frame
#define AUDIO_DMA_BUFFER_SIZE    1024   // samples per DMA buffer
#define AUDIO_DMA_BUFFER_COUNT   4      // number of DMA buffers
#define AUDIO_SAMPLE_DURATION_S  3      // seconds of audio per measurement window

// ── Noise detection ───────────────────────────────────────────────────────
#define NOISE_TARGET_FREQ_HZ     100    // frequency band of interest (Hz)
#define NOISE_DEFAULT_THRESHOLD  65.0f  // dB SPL default; overridden by NVS
#define NOISE_SUSTAIN_COUNT      3      // consecutive over-threshold samples before notify
#define NOISE_SAMPLE_INTERVAL_S  10     // seconds between measurements (light-sleep gap)

// ── Wi-Fi / Captive portal ────────────────────────────────────────────────
#define WIFI_AP_SSID              "RangeHoodSetup"
#define WIFI_AP_PASSWORD          ""              // open AP (no password)
#define WIFI_AP_IP                "192.168.4.1"
#define WIFI_CONNECT_TIMEOUT_S    20              // seconds to wait for STA connection
#define WIFI_RECONNECT_INTERVAL_S 30              // seconds between reconnect attempts
#define WIFI_PORTAL_TIMEOUT_S     300             // 5 min: portal auto-close on no input

// ── IFTTT ─────────────────────────────────────────────────────────────────
#define IFTTT_HOST        "maker.ifttt.com"
#define IFTTT_EVENT_NAME  "range_hood_noise"
// IFTTT API key is stored in NVS (NVS_KEY_IFTTT_KEY), not hardcoded here.

// ── NVS namespace and keys ────────────────────────────────────────────────
#define NVS_NAMESPACE      "hood"
#define NVS_KEY_WIFI_SSID  "ssid"
#define NVS_KEY_WIFI_PASS  "pass"
#define NVS_KEY_IFTTT_KEY  "ifttt_key"
#define NVS_KEY_THRESHOLD  "threshold"

// ── Notification rate-limiting ────────────────────────────────────────────
#define NOTIF_MIN_INTERVAL_MS  60000UL  // minimum ms between IFTTT notifications

// ── Power / button ────────────────────────────────────────────────────────
#define BTN_LONG_PRESS_MS  3000  // ms to hold POWER button for shutdown
#define BTN_DEBOUNCE_MS    50    // debounce window in ms
