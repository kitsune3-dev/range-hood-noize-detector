#pragma once

#include <stdbool.h>

// Send an IFTTT Webhook notification.
//   value1 = measured dB reading (e.g. "72.3 dB")
//   value2 = configured threshold (e.g. "65 dB")
//   value3 = static label "range hood"
// Returns true if the HTTP request succeeded (HTTP 200).
bool notification_send(float db_value, float threshold_db);

// Returns true if enough time has passed since the last notification.
// Enforces a NOTIF_MIN_INTERVAL_MS cooldown to prevent spamming.
bool notification_can_send();
