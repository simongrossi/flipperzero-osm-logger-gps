#pragma once
#include <stdint.h>

typedef struct {
    const char* label;
    const char* key;
    const char* value;
} Preset;

extern const Preset PRESETS[];
extern const uint8_t PRESETS_COUNT;
