#pragma once
#include <stdint.h>

void storage_write_all_formats(
    float lat,
    float lon,
    float altitude,
    float hdop,
    uint8_t sats,
    const char* tag,
    const char* note);

// Compte le nombre de points déjà présents dans points.jsonl (0 si pas de fichier).
uint32_t storage_count_saved_points(void);

// Ajoute un trkpt GPX à track.gpx. Si new_segment=true, insère </trkseg><trkseg>
// avant le point pour délimiter une nouvelle session de tracking.
void storage_append_trkpt(float lat, float lon, float altitude, bool new_segment);
