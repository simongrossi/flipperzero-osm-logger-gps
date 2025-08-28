// storage.c — JSONL + CSV using storage_file_alloc/free (SDK 1.3.x)
#include <furi.h>
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

void storage_write_all_formats(float lat, float lon, float hdop, uint8_t sats,
                               const char* k, const char* v, const char* note) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    ensure_dir(s);

    const char* ts = "2025-08-28T08:45:12Z"; // TODO: inject real timestamp

    char jsonl[320];
    snprintf(
        jsonl, sizeof(jsonl),
        "{\"ts\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"tags\":{\"%s\":\"%s\"},"
        "\"note\":\"%s\",\"hdop\":%.1f,\"sat\":%u}",
        ts, (double)lat, (double)lon, (k?k:""), (v?v:""),
        (note?note:""), (double)hdop, (unsigned)sats
    );
    append_line(s, "/ext/apps_data/osm_logger/points.jsonl", jsonl);

    char csv[320];
    snprintf(
        csv, sizeof(csv),
        "%s,%.6f,%.6f,\"%s=%s | %s\"",
        ts, (double)lat, (double)lon, (k?k:""), (v?v:""), (note?note:"")
    );
    append_line(s, "/ext/apps_data/osm_logger/notes.csv", csv);

    furi_record_close(RECORD_STORAGE);
}
