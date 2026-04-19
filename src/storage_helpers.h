#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

// Supprime le dernier point sauvegardé de TOUS les fichiers (JSONL, CSV, GPX, GeoJSON).
// Retourne true si au moins un fichier a été modifié.
bool storage_delete_last_point(void);

// Supprime tous les fichiers de points (points.jsonl, notes.csv, points.gpx,
// points.geojson). track.gpx n'est pas touché (mode séparé).
void storage_clear_all_points(void);

// Lit les N dernières lignes de points.jsonl, extrait les champs "time" et "tag"
// pour chacune, et les écrit sous la forme "HH:MM  tag" dans out_lines (buffer
// d'au moins N * line_size octets). Retourne le nombre de lignes lues (0..N).
uint8_t storage_read_last_points(char* out_lines, uint8_t max_lines, size_t line_size);

// Lit le N-ième point le plus récent (0 = dernier) en brut (ligne JSONL complète).
// Retourne true si trouvé, false sinon.
bool storage_get_point_raw(uint8_t idx_from_end, char* out, size_t out_size);
