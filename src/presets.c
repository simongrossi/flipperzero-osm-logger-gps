#include "presets.h"

#include <furi.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <string.h>

#define PRESETS_FILE "/ext/apps_data/osm_logger/presets.txt"
#define PRESETS_MAX_FILE_SIZE 4096
#define PRESETS_MAX_ENTRIES 64

// --- Defaults compilés en dur ---
// Note : on peut ajouter des variantes en mettant plusieurs valeurs et en
// ajustant variant_count. Ex. {"Fontaine", "amenity", {"drinking_water", "fountain"}, 2}.
static const Preset DEFAULT_PRESETS[] = {
    // Exemple de variantes mélangées : valeurs alternatives + tags additionnels
    {"Bench", "amenity", {"bench", "material=wood", "material=metal", "material=concrete"}, 4},
    {"Waste basket", "amenity", {"waste_basket"}, 1},
    {"Drinking water", "amenity", {"drinking_water", "fountain", "bottle=yes"}, 3},
    {"Toilets", "amenity", {"toilets", "fee=yes", "wheelchair=yes"}, 3},
    {"Street lamp", "highway", {"street_lamp"}, 1},
    {"Post box", "amenity", {"post_box"}, 1},
    {"Info board", "tourism", {"information"}, 1},
    {"Picnic table", "leisure", {"picnic_table"}, 1},
    {"Tree", "natural", {"tree"}, 1},
    {"Crossing", "highway", {"crossing"}, 1},
    {"Bus stop", "highway", {"bus_stop"}, 1},
    {"Bike parking", "amenity", {"bicycle_parking"}, 1},
    {"Car parking", "amenity", {"parking", "motorcycle_parking"}, 2},
    {"Charging station", "amenity", {"charging_station"}, 1},
    {"Playground", "leisure", {"playground"}, 1},
    {"Basketball hoop", "amenity", {"basketball_hoop"}, 1},
    {"Recycling", "amenity", {"recycling"}, 1},
    {"Pharmacy", "amenity", {"pharmacy"}, 1},
    {"Cafe", "amenity", {"cafe", "pub", "bar", "fast_food"}, 4},
    {"Restaurant", "amenity", {"restaurant"}, 1},
    {"Bakery", "shop", {"bakery"}, 1},
    {"Defibrillator", "emergency", {"defibrillator"}, 1},
    {"Fire hydrant", "emergency", {"fire_hydrant"}, 1},
};
static const uint8_t DEFAULT_PRESETS_COUNT =
    (uint8_t)(sizeof(DEFAULT_PRESETS) / sizeof(DEFAULT_PRESETS[0]));

static char* g_buffer = NULL;
static Preset* g_entries = NULL;
static uint8_t g_count = 0;
static const Preset* g_active = NULL;
static bool g_from_file = false;

// Split une chaîne de valeurs séparées par ',' en N pointeurs (modifie le buffer
// en remplaçant ',' par '\0'). Retourne le nombre de valeurs trouvées.
static uint8_t split_variants(char* value_str, const char* out[]) {
    uint8_t n = 0;
    char* p = value_str;
    if(!p || !*p) return 0;
    out[n++] = p;
    while(*p && n < PRESETS_MAX_VARIANTS) {
        if(*p == ',') {
            *p = '\0';
            p++;
            if(*p) out[n++] = p;
        } else {
            p++;
        }
    }
    return n;
}

// Parse le buffer (modifié en place, ';' et '\n' deviennent '\0').
static uint8_t parse_buffer(char* buf, size_t size, Preset* out, uint8_t max_entries) {
    uint8_t n = 0;
    size_t i = 0;

    while(i < size && n < max_entries) {
        while(i < size && (buf[i] == '\n' || buf[i] == '\r' || buf[i] == ' ' || buf[i] == '\t')) {
            i++;
        }
        if(i >= size) break;

        if(buf[i] == '#') {
            while(i < size && buf[i] != '\n') i++;
            continue;
        }

        char* label = &buf[i];
        while(i < size && buf[i] != ';' && buf[i] != '\n') i++;
        if(i >= size || buf[i] != ';') {
            while(i < size && buf[i] != '\n') i++;
            continue;
        }
        buf[i++] = '\0';

        char* key = &buf[i];
        while(i < size && buf[i] != ';' && buf[i] != '\n') i++;
        if(i >= size || buf[i] != ';') {
            while(i < size && buf[i] != '\n') i++;
            continue;
        }
        buf[i++] = '\0';

        char* values = &buf[i];
        while(i < size && buf[i] != '\n' && buf[i] != '\r') i++;
        if(i < size) buf[i++] = '\0';

        if(*label && *key && *values) {
            Preset p = {0};
            p.label = label;
            p.key = key;
            p.variant_count = split_variants(values, p.variants);
            if(p.variant_count > 0) {
                out[n++] = p;
            }
        }
    }

    return n;
}

static bool try_load_from_file(void) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    File* f = storage_file_alloc(s);
    bool ok = false;

    if(f && storage_file_open(f, PRESETS_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint64_t sz = storage_file_size(f);
        if(sz > 0 && sz <= PRESETS_MAX_FILE_SIZE) {
            char* buf = malloc((size_t)sz + 1);
            if(buf) {
                uint16_t read = storage_file_read(f, buf, (uint16_t)sz);
                buf[read] = '\0';

                Preset* entries = malloc(sizeof(Preset) * PRESETS_MAX_ENTRIES);
                if(entries) {
                    uint8_t n = parse_buffer(buf, read, entries, PRESETS_MAX_ENTRIES);
                    if(n > 0) {
                        g_buffer = buf;
                        g_entries = entries;
                        g_count = n;
                        g_active = entries;
                        g_from_file = true;
                        ok = true;
                        FURI_LOG_I("OSM", "Loaded %u presets from SD", (unsigned)n);
                    } else {
                        free(entries);
                    }
                }
                if(!ok) free(buf);
            }
        } else if(sz > PRESETS_MAX_FILE_SIZE) {
            FURI_LOG_W("OSM", "presets.txt too large (%llu bytes), using defaults", sz);
        }
        storage_file_close(f);
    }

    if(f) storage_file_free(f);
    furi_record_close(RECORD_STORAGE);
    return ok;
}

void presets_init(void) {
    if(try_load_from_file()) return;
    g_active = DEFAULT_PRESETS;
    g_count = DEFAULT_PRESETS_COUNT;
    g_from_file = false;
}

void presets_deinit(void) {
    if(g_from_file) {
        free(g_entries);
        free(g_buffer);
        g_entries = NULL;
        g_buffer = NULL;
    }
    g_active = NULL;
    g_count = 0;
    g_from_file = false;
}

uint8_t presets_count(void) {
    return g_count;
}

const Preset* presets_get(uint8_t idx) {
    if(idx >= g_count || !g_active) return NULL;
    return &g_active[idx];
}

bool presets_loaded_from_file(void) {
    return g_from_file;
}

const char* preset_value(const Preset* p, uint8_t variant_idx) {
    if(!p || p->variant_count == 0) return NULL;
    if(variant_idx >= p->variant_count) variant_idx = 0;
    return p->variants[variant_idx];
}

size_t preset_build_tag(const Preset* p, uint8_t variant_idx, char* out, size_t out_size) {
    if(!p || !out || out_size == 0) return 0;
    const char* v = preset_value(p, variant_idx);
    if(!v) {
        out[0] = '\0';
        return 0;
    }
    // Variante "k=v" -> tag additionnel au primaire
    if(strchr(v, '=')) {
        const char* primary = p->variants[0] ? p->variants[0] : "";
        int n = snprintf(out, out_size, "%s=%s;%s", p->key, primary, v);
        return n > 0 ? (size_t)n : 0;
    }
    // Variante valeur : key=value simple
    int n = snprintf(out, out_size, "%s=%s", p->key, v);
    return n > 0 ? (size_t)n : 0;
}

size_t preset_build_primary_tag(const Preset* p, char* out, size_t out_size) {
    if(!p || !out || out_size == 0) return 0;
    const char* primary = p->variants[0] ? p->variants[0] : "";
    int n = snprintf(out, out_size, "%s=%s", p->key, primary);
    return n > 0 ? (size_t)n : 0;
}
