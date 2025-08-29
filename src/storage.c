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
    for(char* p = s; *p; ++p) if(*p == '"') *p = '\'';
}

void storage_write_all_formats(float lat, float lon, const char* note) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    ensure_dir(s);

    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);

    char iso[32];
    snprintf(
        iso,
        sizeof(iso),
        "%04u-%02u-%02uT%02u:%02u:%02uZ",
        dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    // JSONL
    {
        char buf[256];
        snprintf(
            buf,
            sizeof(buf),
            "{\"time\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"note\":\"%s\"}",
            iso,
            (double)lat,
            (double)lon,
            note ? note : "");
        append_line(s, "/ext/apps_data/osm_logger/points.jsonl", buf);
    }

    // CSV
    {
        char note_copy[96] = {0};
        if(note) {
            strncpy(note_copy, note, sizeof(note_copy)-1);
            sanitize_quotes(note_copy);
        }
        char buf[256];
        snprintf(
            buf,
            sizeof(buf),
            "\"%s\",%.6f,%.6f,\"%s\"",
            iso,
            (double)lat,
            (double)lon,
            note_copy);
        append_line(s, "/ext/apps_data/osm_logger/notes.csv", buf);
    }

    furi_record_close(RECORD_STORAGE);
}
