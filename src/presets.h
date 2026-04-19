#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

// Construit la chaîne de tag effective pour (preset, variant_idx) :
//   - si la variante contient '=' : tag additionnel, résultat = "key=primary;variant"
//     (ex. "amenity=bench;material=wood")
//   - sinon : valeur alternative, résultat = "key=variant" (ex. "amenity=bench")
// Écrit dans `out`, retourne le nombre d'octets écrits (hors null), ou 0 si erreur.
size_t preset_build_tag(const Preset* p, uint8_t variant_idx, char* out, size_t out_size);
