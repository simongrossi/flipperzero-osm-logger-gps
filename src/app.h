#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <gui/view_dispatcher.h>

typedef struct App App;

bool app_get_fix(App* app, double* lat, double* lon, float* hdop, uint8_t* sats);
uint8_t app_get_preset_count(App* app);
const char* app_get_preset_key(App* app, uint8_t idx);
const char* app_get_preset_variant(App* app, uint8_t idx, uint8_t* has_variant);
ViewDispatcher* app_get_view_dispatcher(App* app);

bool app_save_point(App* app, const char* key, const char* variant, const char* note,
                    double lat, double lon, float hdop, uint8_t sats, const char* quality);
