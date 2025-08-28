// storage.c — JSONL + CSV (SDK >1.3.x): append via FSOM_OPEN_APPEND
#include <furi.h>
#include <furi_hal_rtc.h>
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

static bool append_line(Storage* s, const char* path, const char* line) {
    File* f = storage_file_alloc(s);
    if(!f) return false;

    if(!storage_file_open(f, path, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        storage_file_free(f);
        return false;
    }
    
    storage_file_write(f, line, strlen(line));
    storage_file_write(f, "\n", 1);
    
    storage_file_close(f);
    storage_file_free(f);
    return true;
}

static void sanitize_quotes(char* s) {
    if(!s) return;
    for(char* p = s; *p; ++p) if(*p == '\"') *p = '\'';
}

void storage_write_all_formats(float lat, float lon, float hdop, uint8_t sats,
                               const char* k, const char* v, const char* note) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    if(!s) return;
    ensure_dir(s);

    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    // --- CORRECTION 1 : Buffer du timestamp augmenté ---
    char ts[32]; // "YYYY-MM-DDTHH:MM:SSZ" + marge de sécurité
    snprintf(ts, sizeof(ts), "%04u-%02u-%02uT%02u:%02u:%02uZ",
             (unsigned)dt.year, (unsigned)dt.month, (unsigned)dt.day,
             (unsigned)dt.hour, (unsigned)dt.minute, (unsigned)dt.second);

    char key_buf[64] = {0};
    char val_buf[64] = {0};
    char note_buf[160] = {0};
    if(k)  strncpy(key_buf, k,  sizeof(key_buf)-1);
    if(v)  strncpy(val_buf, v,  sizeof(val_buf)-1);
    if(note) strncpy(note_buf, note, sizeof(note_buf)-1);
    sanitize_quotes(key_buf);
    sanitize_quotes(val_buf);
    sanitize_quotes(note_buf);

    // --- CORRECTION 2 : Buffers JSONL et CSV augmentés ---
    char jsonl[512];
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

    char csv[512];
    snprintf(
        csv, sizeof(csv),
        "%s,%.6f,%.6f,\"%s=%s | %s\"",
        ts, (double)lat, (double)lon,
        key_buf, val_buf, note_buf
    );
    append_line(s, "/ext/apps_data/osm_logger/notes.csv", csv);

    furi_record_close(RECORD_STORAGE);
}