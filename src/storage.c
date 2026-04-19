// storage.c — JSONL + CSV (SDK >1.3.x): append via FSOM_OPEN_APPEND
#include <furi.h>
#include <furi_hal_rtc.h>
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static const char* DIR = "/ext/apps_data/osm_logger";

static bool ensure_dir(Storage* s) {
    FileInfo fi;
    if(storage_common_stat(s, DIR, &fi) == FSE_OK) return true;
    FS_Error err = storage_common_mkdir(s, DIR);
    if(err == FSE_OK || err == FSE_EXIST) return true;
    FURI_LOG_E("OSM", "mkdir %s failed: %d", DIR, (int)err);
    return false;
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

// Stratégie GPX / GeoJSON : garder un fichier valide entre deux saves.
// On écrit header + body + footer. Aux saves suivants, on seek juste avant
// le footer, on truncate, puis on écrit (prefix) + body + footer.
static void write_append_framed(
    Storage* s,
    const char* path,
    const char* header,              // utilisé seulement à la création du fichier
    const char* body,                // contenu principal à ajouter
    const char* body_prefix,         // écrit AVANT body si le fichier existait déjà (ex. "," pour séparer)
    const char* footer,
    size_t footer_len) {
    FileInfo fi;
    bool exists = (storage_common_stat(s, path, &fi) == FSE_OK) && (fi.size >= footer_len);

    File* f = storage_file_alloc(s);
    if(!f) return;

    if(!exists) {
        if(storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            if(header) storage_file_write(f, header, strlen(header));
            storage_file_write(f, body, strlen(body));
            if(footer) storage_file_write(f, footer, footer_len);
            storage_file_close(f);
        }
    } else {
        if(storage_file_open(f, path, FSAM_READ_WRITE, FSOM_OPEN_EXISTING)) {
            uint64_t size = storage_file_size(f);
            if(footer && size >= footer_len) {
                storage_file_seek(f, (uint32_t)(size - footer_len), true);
                storage_file_truncate(f);
            }
            if(body_prefix) storage_file_write(f, body_prefix, strlen(body_prefix));
            storage_file_write(f, body, strlen(body));
            if(footer) storage_file_write(f, footer, footer_len);
            storage_file_close(f);
        }
    }

    storage_file_free(f);
}

static const char GPX_HEADER[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gpx version=\"1.1\" creator=\"Flipper Zero OSM Logger\" "
    "xmlns=\"http://www.topografix.com/GPX/1/1\">\n";
static const char GPX_FOOTER[] = "</gpx>\n";

static const char GEOJSON_HEADER[] = "{\"type\":\"FeatureCollection\",\"features\":[\n";
static const char GEOJSON_FOOTER[] = "\n]}\n";

void storage_write_all_formats(
    float lat,
    float lon,
    float altitude,
    float hdop,
    uint8_t sats,
    const char* tag,
    const char* note) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    if(!ensure_dir(s)) {
        furi_record_close(RECORD_STORAGE);
        return;
    }

    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);

    char iso[32];
    snprintf(
        iso,
        sizeof(iso),
        "%04u-%02u-%02uT%02u:%02u:%02uZ",
        dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    const char* tag_s = tag ? tag : "";

    // JSONL
    {
        char buf[320];
        snprintf(
            buf,
            sizeof(buf),
            "{\"time\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,"
            "\"hdop\":%.1f,\"sats\":%u,\"tag\":\"%s\",\"note\":\"%s\"}",
            iso,
            (double)lat,
            (double)lon,
            (double)altitude,
            (double)hdop,
            (unsigned)sats,
            tag_s,
            note ? note : "");
        append_line(s, "/ext/apps_data/osm_logger/points.jsonl", buf);
    }

    // CSV : time,lat,lon,alt,hdop,sats,tag,note
    {
        char note_copy[96] = {0};
        if(note) {
            strncpy(note_copy, note, sizeof(note_copy) - 1);
            note_copy[sizeof(note_copy) - 1] = '\0';
            sanitize_quotes(note_copy);
        }
        char buf[320];
        snprintf(
            buf,
            sizeof(buf),
            "\"%s\",%.6f,%.6f,%.1f,%.1f,%u,\"%s\",\"%s\"",
            iso,
            (double)lat,
            (double)lon,
            (double)altitude,
            (double)hdop,
            (unsigned)sats,
            tag_s,
            note_copy);
        append_line(s, "/ext/apps_data/osm_logger/notes.csv", buf);
    }

    // GPX (waypoint)
    {
        char wpt[320];
        snprintf(
            wpt,
            sizeof(wpt),
            "  <wpt lat=\"%.6f\" lon=\"%.6f\">\n"
            "    <ele>%.1f</ele>\n"
            "    <time>%s</time>\n"
            "    <name>%s</name>\n"
            "    <desc>HDOP=%.1f sats=%u</desc>\n"
            "  </wpt>\n",
            (double)lat,
            (double)lon,
            (double)altitude,
            iso,
            tag_s,
            (double)hdop,
            (unsigned)sats);
        write_append_framed(
            s,
            "/ext/apps_data/osm_logger/points.gpx",
            GPX_HEADER,
            wpt,
            NULL,
            GPX_FOOTER,
            sizeof(GPX_FOOTER) - 1);
    }

    // GeoJSON (FeatureCollection)
    {
        char feat[320];
        snprintf(
            feat,
            sizeof(feat),
            "  {\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
            "\"coordinates\":[%.6f,%.6f,%.1f]},"
            "\"properties\":{\"time\":\"%s\",\"tag\":\"%s\","
            "\"hdop\":%.1f,\"sats\":%u}}",
            (double)lon, // GeoJSON convention: [lon, lat, alt]
            (double)lat,
            (double)altitude,
            iso,
            tag_s,
            (double)hdop,
            (unsigned)sats);
        write_append_framed(
            s,
            "/ext/apps_data/osm_logger/points.geojson",
            GEOJSON_HEADER,
            feat,
            ",\n", // séparateur entre Features si le fichier existait déjà
            GEOJSON_FOOTER,
            sizeof(GEOJSON_FOOTER) - 1);
    }

    furi_record_close(RECORD_STORAGE);
}

uint32_t storage_count_saved_points(void) {
    const char* path = "/ext/apps_data/osm_logger/points.jsonl";
    Storage* s = furi_record_open(RECORD_STORAGE);
    uint32_t count = 0;

    File* f = storage_file_alloc(s);
    if(f && storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char chunk[256];
        uint16_t read;
        do {
            read = storage_file_read(f, chunk, sizeof(chunk));
            for(uint16_t i = 0; i < read; i++) {
                if(chunk[i] == '\n') count++;
            }
        } while(read > 0);
        storage_file_close(f);
    }
    if(f) storage_file_free(f);

    furi_record_close(RECORD_STORAGE);
    return count;
}

static const char TRACK_HEADER[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gpx version=\"1.1\" creator=\"Flipper Zero OSM Logger\" "
    "xmlns=\"http://www.topografix.com/GPX/1/1\">\n"
    "<trk><name>Track</name>\n"
    "<trkseg>\n";
static const char TRACK_FOOTER[] = "</trkseg>\n</trk>\n</gpx>\n";

void storage_append_trkpt(float lat, float lon, float altitude, bool new_segment) {
    Storage* s = furi_record_open(RECORD_STORAGE);
    if(!ensure_dir(s)) {
        furi_record_close(RECORD_STORAGE);
        return;
    }

    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    char iso[32];
    snprintf(
        iso,
        sizeof(iso),
        "%04u-%02u-%02uT%02u:%02u:%02uZ",
        dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    char pt[192];
    snprintf(
        pt,
        sizeof(pt),
        "  <trkpt lat=\"%.6f\" lon=\"%.6f\"><ele>%.1f</ele><time>%s</time></trkpt>\n",
        (double)lat, (double)lon, (double)altitude, iso);

    write_append_framed(
        s,
        "/ext/apps_data/osm_logger/track.gpx",
        TRACK_HEADER,
        pt,
        new_segment ? "</trkseg>\n<trkseg>\n" : NULL,
        TRACK_FOOTER,
        sizeof(TRACK_FOOTER) - 1);

    furi_record_close(RECORD_STORAGE);
}
