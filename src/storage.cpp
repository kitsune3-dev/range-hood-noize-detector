#include "storage.h"
#include "config.h"

#include <Preferences.h>

bool storage_load(AppConfig &dst) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /* readOnly= */ true)) {
        // Namespace not yet created — return defaults.
        dst.wifi_ssid    = "";
        dst.wifi_pass    = "";
        dst.ifttt_key    = "";
        dst.threshold_db = NOISE_DEFAULT_THRESHOLD;
        return true;
    }

    dst.wifi_ssid    = prefs.getString(NVS_KEY_WIFI_SSID, "");
    dst.wifi_pass    = prefs.getString(NVS_KEY_WIFI_PASS, "");
    dst.ifttt_key    = prefs.getString(NVS_KEY_IFTTT_KEY, "");
    dst.threshold_db = prefs.getFloat(NVS_KEY_THRESHOLD, NOISE_DEFAULT_THRESHOLD);

    prefs.end();
    return true;
}

bool storage_save(const AppConfig &src) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /* readOnly= */ false)) {
        return false;
    }

    prefs.putString(NVS_KEY_WIFI_SSID, src.wifi_ssid);
    prefs.putString(NVS_KEY_WIFI_PASS, src.wifi_pass);
    prefs.putString(NVS_KEY_IFTTT_KEY, src.ifttt_key);
    prefs.putFloat(NVS_KEY_THRESHOLD,  src.threshold_db);

    prefs.end();
    return true;
}

bool storage_clear_wifi() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /* readOnly= */ false)) {
        return false;
    }

    prefs.remove(NVS_KEY_WIFI_SSID);
    prefs.remove(NVS_KEY_WIFI_PASS);

    prefs.end();
    return true;
}
