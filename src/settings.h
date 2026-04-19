#pragma once
#include <stdint.h>
#include <stdbool.h>

#define BAUD_RATE_COUNT 6
extern const uint32_t BAUD_RATES[BAUD_RATE_COUNT];
extern const char* const BAUD_LABELS[BAUD_RATE_COUNT];

#define TRACK_INTERVAL_COUNT 5
extern const uint8_t TRACK_INTERVALS_S[TRACK_INTERVAL_COUNT];
extern const char* const TRACK_INTERVAL_LABELS[TRACK_INTERVAL_COUNT];

#define TRACK_MIN_DIST_COUNT 4
extern const uint8_t TRACK_MIN_DISTS_M[TRACK_MIN_DIST_COUNT];
extern const char* const TRACK_MIN_DIST_LABELS[TRACK_MIN_DIST_COUNT];

typedef struct {
    uint32_t baud_rate;
    uint8_t track_interval_s;
    uint8_t track_min_dist_m;  // 0 = désactivé
    bool track_hdop_strict;    // true : rejet des trkpts si HDOP > 2.5
    bool preview_before_save;  // true : Quick Log demande confirmation avant save
} Settings;

void settings_defaults(Settings* s);
void settings_load(Settings* s);
void settings_save(const Settings* s);

uint8_t baud_rate_to_idx(uint32_t baud);
uint8_t track_interval_to_idx(uint8_t seconds);
uint8_t track_min_dist_to_idx(uint8_t meters);

#ifdef __cplusplus
extern "C" {
#endif

typedef struct App App;
void app_start_settings(App* app);
void app_stop_settings(App* app);

#ifdef __cplusplus
}
#endif
