#include "audio.h"
#include "config.h"

#include <Arduino.h>
#include "driver/i2s.h"

// I2S port used for INMP441.
static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;

// ── I2S configuration ──────────────────────────────────────────────────────

static const i2s_config_t s_i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = AUDIO_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = AUDIO_DMA_BUFFER_COUNT,
    .dma_buf_len          = AUDIO_DMA_BUFFER_SIZE,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0,
};

static const i2s_pin_config_t s_i2s_pins = {
    .bck_io_num   = PIN_I2S_SCK,
    .ws_io_num    = PIN_I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_I2S_SD,
};

// ── Goertzel state ─────────────────────────────────────────────────────────

struct GoertzelState {
    float coeff;    // 2 * cos(2π * k / N)
    float s_prev;
    float s_prev2;
};

static void goertzel_init(GoertzelState &g, float target_freq, int sample_rate, int num_samples) {
    int k      = (int)(0.5f + (float)num_samples * target_freq / (float)sample_rate);
    float w    = 2.0f * M_PI * k / (float)num_samples;
    g.coeff    = 2.0f * cosf(w);
    g.s_prev   = 0.0f;
    g.s_prev2  = 0.0f;
}

static void goertzel_process(GoertzelState &g, float sample) {
    float s = sample + g.coeff * g.s_prev - g.s_prev2;
    g.s_prev2 = g.s_prev;
    g.s_prev  = s;
}

static float goertzel_magnitude_sq(const GoertzelState &g) {
    return g.s_prev2 * g.s_prev2
           + g.s_prev * g.s_prev
           - g.coeff * g.s_prev * g.s_prev2;
}

// ── Public API ─────────────────────────────────────────────────────────────

bool audio_init() {
    esp_err_t err = i2s_driver_install(I2S_PORT, &s_i2s_config, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[audio] i2s_driver_install failed: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_PORT, &s_i2s_pins);
    if (err != ESP_OK) {
        Serial.printf("[audio] i2s_set_pin failed: %d\n", err);
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    Serial.println("[audio] I2S initialized");
    return true;
}

AudioResult audio_measure() {
    const int total_samples = AUDIO_SAMPLE_RATE * AUDIO_SAMPLE_DURATION_S;

    GoertzelState goertzel;
    goertzel_init(goertzel, NOISE_TARGET_FREQ_HZ, AUDIO_SAMPLE_RATE, total_samples);

    double sum_sq = 0.0;
    int    count  = 0;

    int32_t dma_buf[AUDIO_DMA_BUFFER_SIZE];

    while (count < total_samples) {
        size_t bytes_read = 0;
        esp_err_t err = i2s_read(I2S_PORT,
                                 dma_buf,
                                 sizeof(dma_buf),
                                 &bytes_read,
                                 portMAX_DELAY);
        if (err != ESP_OK || bytes_read == 0) {
            Serial.printf("[audio] i2s_read error: %d\n", err);
            return {0.0f, 0.0f, false};
        }

        int samples_in_buf = (int)(bytes_read / sizeof(int32_t));
        for (int i = 0; i < samples_in_buf && count < total_samples; i++, count++) {
            // INMP441 data is in the top 24 bits; shift down and normalise to [-1, 1].
            float sample = (float)(dma_buf[i] >> 9) / (float)(1 << 23);
            sum_sq += (double)(sample * sample);
            goertzel_process(goertzel, sample);
        }
    }

    // RMS → dB SPL (reference: full-scale = 0 dBFS).
    float rms    = sqrtf((float)(sum_sq / total_samples));
    float db_spl = (rms > 0.0f) ? (20.0f * log10f(rms) + 120.0f) : 0.0f;

    float band_energy = goertzel_magnitude_sq(goertzel);

    Serial.printf("[audio] RMS=%.5f  dB=%.1f  100Hz_energy=%.2f\n",
                  rms, db_spl, band_energy);

    return {db_spl, band_energy, true};
}

void audio_deinit() {
    i2s_driver_uninstall(I2S_PORT);
    Serial.println("[audio] I2S released");
}
