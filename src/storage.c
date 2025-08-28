// storage.c — JSONL + CSV (SDK 1.3.x): append via seek, RTC DateTime
#include <furi.h>
#include <furi_hal_rtc.h>          // <-- header RTC correct (DateTime)
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static const char* DIR = "/ext/apps_data/osm_logger";

static void ensure_dir(Storage* s) {
    FileInfo fi;
    if(storage_common_stat(s, DIR, &fi) != FSE_OK) {
        storage_common_mkdir(s, DIR);
    }
}

// Append "à la main": open write, seek end, write, newline, close
static bool append_line(Storage* s, const char* path, const char* line) {
    bool ok = false;
    File* f = storage_file_alloc(s);
    if(!f) return false;

    // SDK 1.3.x: 3 params (no open_mode)
    if(storage_file_open(f, path, FSAM_WRITE)) {
        // Aller en fin de fichier: 2 façons selon SDK;
        // celle-ci (avec 'from_end=true') est supportée en 1.3.x:
        storage_file_seek(f, 0, true);
        // Ecrire la ligne + \n
        storage_file_write(f, line, strlen(line));
        storage_file_write(f, "\n", 1);
        storage_file_close(f);
        ok = true;
    }
    storage_file_free(f);
    return ok;
}

// Remplace les " par ' pour ne pas casser JSON/CSV minimal
static void sanitize_quotes(char* s) {
    if(!s) return;
    for(char* p = s; *p; ++p) if(*p == '\"') *p = '\'';
}

void storage_write_all_formats(float lat, float lon, float hdop, uint8_t sats,
                               const char* k, const char* v, const char* note) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    if(!s) return;
    ensure_dir(s);

    // Horodatage ISO depuis le RTC
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    char ts[21]; // "YYYY-MM-DDTHH:MM:SSZ"
    snprintf(ts, sizeof(ts), "%04u-%02u-%02uT%02u:%02u:%02uZ",
             (unsigned)dt.year, (unsigned)dt.month, (unsigned)dt.day,
             (unsigned)dt.hour, (unsigned)dt.minute, (unsigned)dt.second);

    // Buffers modifiables pour sanitization
    char key_buf[64] = {0};
    char val_buf[64] = {0};
    char note_buf[160] = {0};
    if(k)  strncpy(key_buf, k,  sizeof(key_buf)-1);
    if(v)  strncpy(val_buf, v,  sizeof(val_buf)-1);
    if(note) strncpy(note_buf, note, sizeof(note_buf)-1);
    sanitize_quotes(key_buf);
    sanitize_quotes(val_buf);
    sanitize_quotes(note_buf);

    // JSONL (source de vérité)
    char jsonl[320];
    snprintf(
        jsonl, sizeof(jsonl),
        "{\"ts\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,"
        "\"tags\":{\"%s\":\"%s\"},"
        "\"note\":\"%s\",\"hdop\":%.1f,\"sat\":%u}",
        ts,
        (double)lat, (double)lon,
        key_buf, val_buf,
        note_buf, (double)hdop, (unsigned)sats
    );
    append_line(s, "/ext/apps_data/osm_logger/points.jsonl", jsonl);

    // CSV de notes
    char csv[320];
    snprintf(
        csv, sizeof(csv),
        "%s,%.6f,%.6f,\"%s=%s | %s\"",
        ts, (double)lat, (double)lon,
        key_buf, val_buf, note_buf
    );
    append_line(s, "/ext/apps_data/osm_logger/notes.csv", csv);

    // Pas de GPX/GeoJSON ici (conversion offline)
    furi_record_close(RECORD_STORAGE);
}
