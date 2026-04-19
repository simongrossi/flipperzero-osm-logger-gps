#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PRESETS_MAX_VARIANTS 8

typedef struct {
    const char* label;
    const char* key;
    const char* variants[PRESETS_MAX_VARIANTS]; // [0] = valeur primaire
    uint8_t variant_count;                      // >= 1
} Preset;

// Charge les presets depuis /ext/apps_data/osm_logger/presets.txt si présent,
// sinon retombe sur la liste par défaut compilée en dur.
void presets_init(void);

// Libère toute mémoire allouée (no-op si defaults).
void presets_deinit(void);

uint8_t presets_count(void);
const Preset* presets_get(uint8_t idx); // NULL si idx hors bornes
bool presets_loaded_from_file(void);

// Retourne la valeur correspondant à la variante donnée, ou la primaire si
// variant_idx >= variant_count.
const char* preset_value(const Preset* p, uint8_t variant_idx);
