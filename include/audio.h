#pragma once

#include <stdint.h>
#include <stdbool.h>

struct AudioResult {
    float db_spl;            // average dB SPL over the sample window
    float band_energy_100hz; // Goertzel energy at the 100 Hz band
    bool  valid;             // false if I2S read failed
};

// Initialize I2S peripheral and DMA buffers.
// Returns true on success.
bool audio_init();

// Capture audio for AUDIO_SAMPLE_DURATION_S seconds, apply Goertzel
// filter at NOISE_TARGET_FREQ_HZ, and return the averaged result.
// Returns an AudioResult with valid=false on I2S error.
AudioResult audio_measure();

// Release I2S peripheral (call before entering light sleep).
void audio_deinit();
